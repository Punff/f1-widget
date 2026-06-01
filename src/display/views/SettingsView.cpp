#include "SettingsView.h"
#include "../DisplayManager.h"
#include "../../time/TimeManager.h"
#include "../../../include/UI_Fonts.h"
#include <LittleFS.h>
#include <esp_system.h>

#include "../../data/DataCache.h"

extern TimeManager *timeMgr;
extern DataCache *cache;

static constexpr uint32_t SETTINGS_MAGIC = 0x5E772; // Incremented for favTeamId
static constexpr const char *SETTINGS_PATH = "/settings.bin";

static constexpr int COL_ICON = 15;
static constexpr int COL_LABEL = 70;
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
                UI::COL_ACCENT = UI::rgb565to888(cs.team.teamColor);
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
    case SET_SYSINFO:
        icon = "SYS";
        break;
    case SET_CLEAR_CACHE:
        icon = "CLR";
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
    _rowSprite->drawString(icon, COL_ICON, _rowH / 2);

    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(dim);

    switch ((SettingIdx)dataIdx)
    {
    case SET_BRIGHTNESS:
        _rowSprite->drawString("Backlight", COL_LABEL, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(COL_VALUE - 100, 0, 120, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->drawNumber(_settings.brightness * 100 / 255, COL_VALUE, _rowH / 2);
        _rowSprite->drawString("%", COL_VALUE + 30, _rowH / 2);
        break;
    case SET_UTC_OFFSET:
        _rowSprite->drawString("UTC Offset", COL_LABEL, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(COL_VALUE - 100, 0, 120, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "UTC%+d", _settings.utcOffset);
            _rowSprite->drawString(buf, COL_VALUE, _rowH / 2);
        }
        break;
    case SET_FAV_TEAM:
        _rowSprite->drawString("Fav Team", COL_LABEL, _rowH / 2);
        if (_editing)
        {
            _rowSprite->fillRect(COL_VALUE - 150, 0, 170, _rowH, UI::COL_BG_SEL);
            _rowSprite->setTextColor(UI::COL_ACCENT);
        }
        _rowSprite->setTextDatum(middle_right);
        if (_settings.favTeamId[0] == '\0') {
            _rowSprite->drawString("None", COL_VALUE, _rowH / 2);
        } else {
            bool found = false;
            if (cache) {
                for (const auto& cs : cache->constructorStandings) {
                    if (strcmp(cs.team.id, _settings.favTeamId) == 0) {
                        _rowSprite->drawString(cs.team.name, COL_VALUE, _rowH / 2);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) _rowSprite->drawString(_settings.favTeamId, COL_VALUE, _rowH / 2);
        }
        break;
    case SET_SYSINFO:
        _rowSprite->drawString("System Info", COL_LABEL, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(UI::COL_TEXT_DIM);
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%u KB", ESP.getFreeHeap() / 1024);
            _rowSprite->drawString(buf, COL_VALUE, _rowH / 2);
        }
        break;
    case SET_CLEAR_CACHE:
        _rowSprite->drawString("Clear Cache & Reboot", COL_LABEL, _rowH / 2);
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(UI::COL_ACCENT);
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

void SettingsView::drawFooter()
{
    _dm->footer()->draw();
    char buf[32];
    snprintf(buf, sizeof(buf), "SET \xc2\xb7 Heap: %uK", ESP.getFreeHeap() / 1024);
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
        applyFavTeamColor(_settings);
        fullRedraw();
        return;
    }
    switch ((SettingIdx)_cursor)
    {
    case SET_BRIGHTNESS:
    case SET_UTC_OFFSET:
    case SET_FAV_TEAM:
        _editing = true;
        partialRedraw(_cursor);
        break;
    case SET_CLEAR_CACHE:
        cache->clear();
        saveSettings(_settings);
        delay(500);
        ESP.restart();
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
        applyBrightness();
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
void SettingsView::applyBrightness() { ledcWrite(0, _settings.brightness); }
