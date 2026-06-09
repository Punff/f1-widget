#include "SettingsView.h"
#include "WifiSettingsView.h"
#include "DataSettingsView.h"
#include "../DisplayManager.h"
#include "../../time/TimeManager.h"
#include "../../../include/UI_Fonts.h"
#include "../../config.h"
#include <LittleFS.h>
#include <esp_system.h>

#include "../../data/DataCache.h"
#include "../../wifi/WiFiManager.h"

extern TimeManager *timeMgr;
extern DataCache *cache;

static constexpr uint32_t SETTINGS_MAGIC = 0x5E773; // Incremented for displayTimeoutSec
static constexpr const char *SETTINGS_PATH = "/settings.bin";

// Column offsets (using shared UI constants)

void SettingsView::loadSettings(SettingsData &s)
{
    if (LittleFS.exists(SETTINGS_PATH))
    {
        File f = LittleFS.open(SETTINGS_PATH, FILE_READ);
        if (f)
        {
            if (f.read((uint8_t *)&s, sizeof(SettingsData)) == sizeof(SettingsData) && s.magic == SETTINGS_MAGIC)
            {
                f.close();
                return;
            }
            f.close();
        }
    }
    s.magic = SETTINGS_MAGIC;
    s.brightness = 255;
    s.utcOffset = 2;
    s.displayTimeoutSec = 0;
    s.favTeamId[0] = '\0';
}

void SettingsView::applyFavTeamColor(const SettingsData &s)
{
    UI::COL_ACCENT = UI::COL_ACCENT_DEFAULT;
    if (s.favTeamId[0] != '\0' && cache)
    {
        for (const auto &cs : cache->constructorStandings)
        {
            if (strcmp(cs.team.id, s.favTeamId) == 0)
            {
                uint32_t c = UI::rgb565to888(cs.team.teamColor);
                // Reject colors too dark to see on COL_BG
                uint8_t r = (c >> 16) & 0xFF;
                uint8_t g = (c >> 8) & 0xFF;
                uint8_t b = c & 0xFF;
                if (r > 96 || g > 96 || b > 96)
                    UI::COL_ACCENT = c;
                break;
            }
        }
    }
}

void SettingsView::saveSettings(const SettingsData &s)
{
    File f = LittleFS.open(SETTINGS_PATH, FILE_WRITE);
    if (f)
    {
        f.write((uint8_t *)&s, sizeof(SettingsData));
        f.close();
    }
}

SettingsView::SettingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2), _editing(false)
{
    loadSettings(_settings);
}

int SettingsView::dataSize() const { return SET_COUNT; }

void SettingsView::onEnter()
{
    loadSettings(_settings);
    timeMgr->setUTCOffset(_settings.utcOffset);
    _editing = false;
    ScrollListView::onEnter();
}

void SettingsView::drawHeader() { _dm->header()->draw("SYSTEM SETTINGS"); }

void SettingsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= SET_COUNT)
        return;

    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_ACCENT);
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    const char *icon = "";
    switch ((SettingIdx)dataIdx)
    {
    case SET_BRIGHTNESS:
        icon = "BRI";
        break;
    case SET_UTC_OFFSET:
        icon = "UTC";
        break;
    case SET_FAV_TEAM:
        icon = "FAV";
        break;
    case SET_DISPLAY_TIMEOUT:
        icon = "DTO";
        break;
    case SET_WIFI:
        icon = "WIF";
        break;
    case SET_DATA:
        icon = "DAT";
        break;
    case SET_ABOUT:
        icon = "ABT";
        break;
    default:
        break;
    }

    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
    _rowSprite->drawString(icon, UI::COL_POS, _rowH / 2);

    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(dim);

    switch ((SettingIdx)dataIdx)
    {
    case SET_BRIGHTNESS:
        _rowSprite->drawString("Backlight", UI::COL_PRIMARY, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(UI::COL_END_R - 100, 0, 120, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->drawNumber(_settings.brightness * 100 / 255, UI::COL_END_R, _rowH / 2);
        _rowSprite->drawString("%", UI::COL_END_R + 30, _rowH / 2);
        break;
    case SET_UTC_OFFSET:
        _rowSprite->drawString("UTC Offset", UI::COL_PRIMARY, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(UI::COL_END_R - 100, 0, 120, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "UTC%+d", _settings.utcOffset);
            _rowSprite->drawString(buf, UI::COL_END_R, _rowH / 2);
        }
        break;
    case SET_FAV_TEAM:
        _rowSprite->drawString("Fav Team", UI::COL_PRIMARY, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(UI::COL_END_R - 150, 0, 170, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        if (_settings.favTeamId[0] == '\0') {
            _rowSprite->drawString("None", UI::COL_END_R, _rowH / 2);
        } else {
            bool found = false;
            if (cache) {
                for (const auto& cs : cache->constructorStandings) {
                    if (strcmp(cs.team.id, _settings.favTeamId) == 0) {
                        _rowSprite->drawString(cs.team.name, UI::COL_END_R, _rowH / 2);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) _rowSprite->drawString(_settings.favTeamId, UI::COL_END_R, _rowH / 2);
        }
        break;
    case SET_DISPLAY_TIMEOUT:
    {
        _rowSprite->drawString("Display Timeout", UI::COL_PRIMARY, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(UI::COL_END_R - 100, 0, 120, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        static const int opts[] = {0, 30, 60, 120, 300};
        static const char *labels[] = {"Never", "30s", "60s", "2m", "5m"};
        int idx = 0;
        for (int i = 0; i < 5; i++) { if (_settings.displayTimeoutSec == opts[i]) { idx = i; break; } }
        _rowSprite->drawString(labels[idx], UI::COL_END_R, _rowH / 2);
        break;
    }
    case SET_WIFI:
        _rowSprite->drawString("WiFi Networks", UI::COL_PRIMARY, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(dim);
        if (wifi_has_saved())
            _rowSprite->drawString("SAVED", UI::COL_END_R, _rowH / 2);
        else
            _rowSprite->drawString("NONE", UI::COL_END_R, _rowH / 2);
        break;
    case SET_DATA:
        _rowSprite->drawString("Data Management", UI::COL_PRIMARY, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("PRESS", UI::COL_END_R, _rowH / 2);
        break;
    case SET_ABOUT:
        _rowSprite->drawString("About", UI::COL_PRIMARY, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString(APP_VERSION, UI::COL_END_R, _rowH / 2);
        break;
    default:
        break;
    }
}

void SettingsView::drawFooter()
{
    _dm->footer()->draw();
    char buf[32];
    snprintf(buf, sizeof(buf), "SET \xc2\xb7 %d/%d", _cursor + 1, SET_COUNT);
    _dm->footer()->drawCenter(buf, UI::COL_MUTED);
}

void SettingsView::onPress()
{
    if (_editing)
    {
        _editing = false;
        timeMgr->setUTCOffset(_settings.utcOffset);
        saveSettings(_settings);
        applyBrightness();
        _dm->setDisplayTimeout(_settings.displayTimeoutSec);
        applyFavTeamColor(_settings);
        fullRedraw();
        return;
    }
    switch ((SettingIdx)_cursor)
    {
    case SET_WIFI:
        _dm->launchWifiSettings();
        break;
    case SET_DATA:
        _dm->launchDataSettings();
        break;
    case SET_BRIGHTNESS:
    case SET_UTC_OFFSET:
    case SET_FAV_TEAM:
    case SET_DISPLAY_TIMEOUT:
        _editing = true;
        partialRedraw(_cursor);
        break;
    default:
        break;
    }
}

void SettingsView::onTurnRight()
{
    if (_editing)
        modifyValue(1);
    else
        ScrollListView::onTurnRight();
}
void SettingsView::onTurnLeft()
{
    if (_editing)
        modifyValue(-1);
    else
        ScrollListView::onTurnLeft();
}
void SettingsView::onLongPress()
{
    if (_editing)
    {
        _editing = false;
        loadSettings(_settings);
        timeMgr->setUTCOffset(_settings.utcOffset);
        applyBrightness();
        _dm->setDisplayTimeout(_settings.displayTimeoutSec);
        applyFavTeamColor(_settings);
        fullRedraw();
    }
    else
        _dm->returnToMenu();
}

void SettingsView::modifyValue(int delta)
{
    switch ((SettingIdx)_cursor)
    {
    case SET_BRIGHTNESS:
    {
        int v = _settings.brightness + delta * 25;
        _settings.brightness = constrain(v, 1, 255);
        applyBrightness();
        drawSingleRow(_cursor - _scrollOffset);
        break;
    }
    case SET_UTC_OFFSET:
    {
        int v = _settings.utcOffset + delta;
        _settings.utcOffset = constrain(v, -12, 14);
        timeMgr->setUTCOffset(_settings.utcOffset);
        drawSingleRow(_cursor - _scrollOffset);
        break;
    }
    case SET_DISPLAY_TIMEOUT:
    {
        static const int opts[] = {0, 30, 60, 120, 300};
        int idx = 0;
        for (int i = 0; i < 5; i++) { if (_settings.displayTimeoutSec == opts[i]) { idx = i; break; } }
        idx = (idx + delta + 5) % 5;
        _settings.displayTimeoutSec = opts[idx];
        _dm->setDisplayTimeout(_settings.displayTimeoutSec);
        drawSingleRow(_cursor - _scrollOffset);
        break;
    }
    case SET_FAV_TEAM:
    {
        if (!cache) break;
        int numTeams = cache->constructorStandings.size();
        if (numTeams == 0) break;

        int currentIdx = -1; // -1 represents "None"
        if (_settings.favTeamId[0] != '\0') {
            for (int i = 0; i < numTeams; i++) {
                if (strcmp(cache->constructorStandings[i].team.id, _settings.favTeamId) == 0) {
                    currentIdx = i;
                    break;
                }
            }
        }

        currentIdx += delta;
        if (currentIdx < -1) currentIdx = numTeams - 1;
        if (currentIdx >= numTeams) currentIdx = -1;

        if (currentIdx == -1) {
            _settings.favTeamId[0] = '\0';
        } else {
            strlcpy(_settings.favTeamId, cache->constructorStandings[currentIdx].team.id, sizeof(_settings.favTeamId));
        }

        drawSingleRow(_cursor - _scrollOffset);
        break;
    }
    default:
        break;
    }
}
void SettingsView::applyBrightness() {
    ledcWrite(0, _settings.brightness);
    _dm->setUserBrightness(_settings.brightness);
}
