#include "DataSettingsView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"
#include "../../api/APIClient.h"
#include "../../data/DataCache.h"
#include "../../time/TimeManager.h"
#include "../../config.h"
#include <LittleFS.h>
#include <esp_system.h>

extern APIClient *api;
extern TimeManager *timeMgr;
extern DataCache *cache;

static const char *SYNC_TIME_PATH = "/data_sync_time.bin";

static time_t loadLastSync()
{
    if (!LittleFS.exists(SYNC_TIME_PATH)) return 0;
    File f = LittleFS.open(SYNC_TIME_PATH, FILE_READ);
    if (!f) return 0;
    time_t t = 0;
    f.read((uint8_t *)&t, sizeof(t));
    f.close();
    return t;
}

static void saveLastSync(time_t t)
{
    File f = LittleFS.open(SYNC_TIME_PATH, FILE_WRITE);
    if (!f) return;
    f.write((uint8_t *)&t, sizeof(t));
    f.close();
}

DataSettingsView::DataSettingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2), _syncing(false), _syncStart(0), _lastSyncTime(0) {}

int DataSettingsView::dataSize() const
{
    return ROW_COUNT;
}

void DataSettingsView::onEnter()
{
    _lastSyncTime = loadLastSync();
    _syncing = false;
    ScrollListView::onEnter();
}

void DataSettingsView::drawHeader()
{
    _dm->header()->draw("DATA MANAGEMENT");
}

void DataSettingsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_ACCENT);
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    switch ((DataRow)dataIdx)
    {
    case ROW_SYNC_ALL:
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
        _rowSprite->setTextDatum(middle_left);
        _rowSprite->drawString("\xe2\x86\xbb", UI::COL_POS, _rowH / 2);

        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("Sync All", UI::COL_PRIMARY, _rowH / 2);

        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(UI::COL_MUTED);
        if (_syncing)
        {
            int phase = (millis() / 500) % 4;
            const char *spinner = "|/-\\";
            char sp[2] = {spinner[phase], 0};
            _rowSprite->drawString(sp, UI::COL_END_R, _rowH / 2);
        }
        else if (_lastSyncTime > 100000)
        {
            struct tm t;
            localtime_r(&_lastSyncTime, &t);
            char buf[20];
            strftime(buf, sizeof(buf), "%H:%M", &t);
            _rowSprite->drawString(buf, UI::COL_END_R, _rowH / 2);
        }
        else
        {
            _rowSprite->drawString("NEVER", UI::COL_END_R, _rowH / 2);
        }
        break;
    }

    case ROW_SYSINFO:
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
        _rowSprite->setTextDatum(middle_left);
        _rowSprite->drawString("SYS", UI::COL_POS, _rowH / 2);

        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("System Info", UI::COL_PRIMARY, _rowH / 2);

        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(dim);
        char buf[16];
        snprintf(buf, sizeof(buf), "%u KB", ESP.getFreeHeap() / 1024);
        _rowSprite->drawString(buf, UI::COL_END_R, _rowH / 2);
        break;
    }

    case ROW_CLEAR_CACHE:
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(selected ? UI::COL_ACCENT : dim);
        _rowSprite->setTextDatum(middle_left);
        _rowSprite->drawString("CLR", UI::COL_POS, _rowH / 2);

        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("Clear Cache & Reboot", UI::COL_PRIMARY, _rowH / 2);

        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("PRESS", UI::COL_END_R, _rowH / 2);
        break;
    }
    }
}

void DataSettingsView::drawFooter()
{
    _dm->footer()->draw();
    char buf[32];
    snprintf(buf, sizeof(buf), "DATA \xc2\xb7 %d/%d", _cursor + 1, ROW_COUNT);
    _dm->footer()->drawCenter(buf, UI::COL_MUTED);
}

void DataSettingsView::onPress()
{
    DataRow row = (DataRow)_cursor;
    if (row == ROW_SYNC_ALL)
    {
        if (_syncing || !api) return;
        _syncing = true;
        _syncStart = millis();
        drawSingleRow(0);
        _dm->footer()->markDirty();
        drawFooter();
        return;
    }

    if (row == ROW_CLEAR_CACHE)
    {
        cache->clear();
        LittleFS.remove(SYNC_TIME_PATH);
        delay(500);
        ESP.restart();
        return;
    }
}

void DataSettingsView::onLongPress()
{
    if (_syncing) return;
    _dm->returnToPrevious();
}

void DataSettingsView::tick()
{
    if (_syncing)
    {
        bool ok = api->syncAll();
        _syncing = false;
        if (timeMgr)
            _lastSyncTime = timeMgr->getLocalTime();
        else
            _lastSyncTime = time(nullptr) + 7200;
        saveLastSync(_lastSyncTime);
        fullRedraw();
        return;
    }
    ScrollListView::tick();
}
