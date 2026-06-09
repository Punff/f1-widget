#include "WifiSettingsView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"
#include "../../wifi/WiFiManager.h"
#include "../../time/TimeManager.h"
#include <WiFi.h>

extern TimeManager *timeMgr;

WifiSettingsView::WifiSettingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2), _numNetworks(0), _connecting(false), _connectingStart(0) {}

void WifiSettingsView::onEnter()
{
    _connecting = false;
    _numNetworks = wifi_saved_count();
    if (_cursor >= dataSize()) _cursor = 0;
    ScrollListView::onEnter();
}

int WifiSettingsView::dataSize() const
{
    return _numNetworks + ROW_COUNT_BASE;
}

void WifiSettingsView::drawHeader()
{
    _dm->header()->draw("WiFi SETTINGS");
}

void WifiSettingsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_ACCENT);
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(dim);

    if (dataIdx < _numNetworks)
    {
        // Saved AP row
        char ssid[32] = {}, pw[64] = {};
        wifi_get_saved(dataIdx, ssid, sizeof(ssid), pw, sizeof(pw));

        // Icon: lock / signal
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
        _rowSprite->drawString("\xe2\x97\x8b", UI::COL_POS, _rowH / 2);

        // SSID name
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString(ssid, UI::COL_PRIMARY, _rowH / 2);

        // Connected indicator
        _rowSprite->setTextDatum(middle_right);
        if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid)
        {
            _rowSprite->setTextColor(UI::COL_ACCENT);
            _rowSprite->drawString("CONNECTED", UI::COL_END_R, _rowH / 2);
        }
    }
    else if (dataIdx == _numNetworks + ROW_ADD_HOTSPOT)
    {
        // Add Network
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
        _rowSprite->drawString("+", UI::COL_POS, _rowH / 2);

        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("Add Network (Hotspot)", UI::COL_PRIMARY, _rowH / 2);
    }
    else if (dataIdx == _numNetworks + ROW_FORGET)
    {
        // Forget Network
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
        _rowSprite->drawString("X", UI::COL_POS, _rowH / 2);

        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("Forget Selected", UI::COL_PRIMARY, _rowH / 2);
    }
}

void WifiSettingsView::drawFooter()
{
    _dm->footer()->draw();
    const char *status = _connecting ? "Connecting..." : "WIFI \xc2\xb7 Saved Networks";
    _dm->footer()->drawCenter(status, UI::COL_MUTED);
}

void WifiSettingsView::onPress()
{
    if (_connecting) return;

    if (_cursor < _numNetworks)
    {
        // Connect to saved AP
        char ssid[32] = {}, pw[64] = {};
        wifi_get_saved(_cursor, ssid, sizeof(ssid), pw, sizeof(pw));

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, pw);
        _connecting = true;
        _connectingStart = millis();
        _dm->footer()->markDirty();
        fullRedraw();
        return;
    }

    if (_cursor == _numNetworks + ROW_ADD_HOTSPOT)
    {
        // Show hotspot instructions before blocking portal
        _tft->fillScreen(UI::COL_BG);
        _tft->setTextDatum(middle_center);
        _tft->setTextColor(UI::COL_ACCENT);
        _tft->setFont(UI::Fonts::HEADER_BIG);
        _tft->drawString("WiFi SETUP", UI::SCREEN_W / 2, 70);

        _tft->setTextColor(UI::COL_TEXT);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString("1. Connect to  \"F1Widget\"", UI::SCREEN_W / 2, 140);
        _tft->drawString("   from your phone or computer", UI::SCREEN_W / 2, 165);

        _tft->drawString("2. Open a browser to", UI::SCREEN_W / 2, 205);
        _tft->drawString("   192.168.4.1", UI::SCREEN_W / 2, 230);

        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString("Select your WiFi network and enter the password", UI::SCREEN_W / 2, 275);

        wifi_start_hotspot();
        // After hotspot closes, refresh
        _numNetworks = wifi_saved_count();
        _connecting = false;
        fullRedraw();
        return;
    }

    if (_cursor == _numNetworks + ROW_FORGET)
    {
        // Forget the first saved network (if any)
        if (_numNetworks > 0)
        {
            wifi_forget(0);
            _numNetworks = wifi_saved_count();
            if (_cursor >= dataSize()) _cursor = dataSize() - 1;
            fullRedraw();
        }
        return;
    }
}

void WifiSettingsView::onLongPress()
{
    if (_connecting)
    {
        WiFi.disconnect(false, true);
        _connecting = false;
        fullRedraw();
        return;
    }
    _dm->returnToPrevious();
}

void WifiSettingsView::tick()
{
    if (_connecting)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            _connecting = false;
            wifi_save_current();
            _dm->footer()->setWifiConnected(true);
            if (timeMgr && !timeMgr->isSynced())
            {
                _dm->footer()->drawCenter("Syncing clock...", UI::COL_MUTED);
                timeMgr->syncNTP();
            }
            fullRedraw();
            return;
        }
        if (millis() - _connectingStart > 15000)
        {
            _connecting = false;
            WiFi.disconnect(false, true);
            fullRedraw();
            return;
        }
    }
    ScrollListView::tick();
}
