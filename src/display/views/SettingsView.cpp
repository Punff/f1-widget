#include "SettingsView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"
#include <LittleFS.h>
#include <esp_system.h>

static constexpr uint32_t SETTINGS_MAGIC = 0x5E771;
static constexpr const char *SETTINGS_PATH = "/settings.bin";

static constexpr int COL_LABEL = 15;
static constexpr int COL_VALUE = 465;

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
    _settings.magic = SETTINGS_MAGIC;
    _settings.brightness = 255;
    _settings.utcOffset = 2;
}

int SettingsView::dataSize() const
{
    return SET_COUNT;
}

void SettingsView::onEnter()
{
    loadSettings(_settings);
    _editing = false;
    ScrollListView::onEnter();
}

void SettingsView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("SYSTEM SETTINGS", 75, 12);

    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);
}

void SettingsView::drawRow(int dataIdx, bool selected, int dist)
{
    (void)dist;
    if (dataIdx < 0 || dataIdx >= SET_COUNT)
        return;

    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }

    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(UI::COL_TEXT);

    switch ((SettingIdx)dataIdx)
    {
    case SET_BRIGHTNESS:
        _rowSprite->drawString("Backlight", COL_LABEL, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(COL_VALUE - 60, 0, 80, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_F1_RED);
        }
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->drawNumber(_settings.brightness * 100 / 255, COL_VALUE, _rowH / 2);
        _rowSprite->drawString("%", COL_VALUE + 30, _rowH / 2);
        break;

    case SET_UTC_OFFSET:
        _rowSprite->drawString("UTC Offset", COL_LABEL, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(COL_VALUE - 60, 0, 80, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_F1_RED);
        }
        _rowSprite->setTextDatum(middle_right);
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "UTC%+d", _settings.utcOffset);
            _rowSprite->drawString(buf, COL_VALUE, _rowH / 2);
        }
        break;

    case SET_SYSINFO:
    {
        _rowSprite->drawString("System Info", COL_LABEL, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(UI::COL_TEXT_DIM);
        char buf[16];
        snprintf(buf, sizeof(buf), "%u KB", ESP.getFreeHeap() / 1024);
        _rowSprite->drawString(buf, COL_VALUE, _rowH / 2);
        break;
    }

    case SET_CLEAR_CACHE:
        _rowSprite->drawString("Clear Cache & Reboot", COL_LABEL, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(UI::COL_F1_RED);
        _rowSprite->drawString("PRESS", COL_VALUE, _rowH / 2);
        break;

    case SET_ABOUT:
        _rowSprite->drawString("About", COL_LABEL, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(UI::COL_TEXT_DIM);
        _rowSprite->drawString("v1.0", COL_VALUE, _rowH / 2);
        break;

    default:
        break;
    }
}

void SettingsView::onPress()
{
    if (_editing)
    {
        _editing = false;
        saveSettings(_settings);
        applyBrightness();
        fullRedraw();
        return;
    }

    switch ((SettingIdx)_cursor)
    {
    case SET_BRIGHTNESS:
    case SET_UTC_OFFSET:
        _editing = true;
        partialRedraw(_cursor);
        break;

    case SET_CLEAR_CACHE:
        cache->clear();
        saveSettings(_settings);
        LittleFS.remove("/settings.bin");
        delay(500);
        ESP.restart();
        break;

    case SET_SYSINFO:
    case SET_ABOUT:
    default:
        break;
    }
}

void SettingsView::onTurnRight()
{
    if (_editing)
    {
        modifyValue(1);
    }
    else
    {
        ScrollListView::onTurnRight();
    }
}

void SettingsView::onTurnLeft()
{
    if (_editing)
    {
        modifyValue(-1);
    }
    else
    {
        ScrollListView::onTurnLeft();
    }
}

void SettingsView::onLongPress()
{
    if (_editing)
    {
        _editing = false;
        loadSettings(_settings);
        applyBrightness();
        fullRedraw();
    }
    else
    {
        _dm->returnToMenu();
    }
}

void SettingsView::modifyValue(int delta)
{
    switch ((SettingIdx)_cursor)
    {
    case SET_BRIGHTNESS:
    {
        int v = _settings.brightness + delta * 25;
        if (v < 0)
            v = 1;
        if (v > 255)
            v = 255;
        _settings.brightness = v;
        applyBrightness();
        drawSingleRow(_cursor - _scrollOffset);
        break;
    }
    case SET_UTC_OFFSET:
    {
        int v = _settings.utcOffset + delta;
        if (v >= -12 && v <= 14)
        {
            _settings.utcOffset = v;
            drawSingleRow(_cursor - _scrollOffset);
        }
        break;
    }
    default:
        break;
    }
}

void SettingsView::applyBrightness()
{
    ledcWrite(0, _settings.brightness);
}
