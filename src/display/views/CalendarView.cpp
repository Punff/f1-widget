#include "CalendarView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"
#include "../../time/TimeManager.h"
#include "../DisplayManager.h"

extern DataCache *cache;
extern TimeManager *timeMgr;
static int nextRoundIdx = -1;

// Portable timegm replacement
static time_t my_timegm(struct tm *t)
{
    return t->tm_sec + t->tm_min * 60 + t->tm_hour * 3600 + t->tm_yday * 86400 +
           (t->tm_year - 70) * 31536000 + ((t->tm_year - 69) / 4) * 86400 -
           ((t->tm_year - 1) / 100) * 86400 + ((t->tm_year + 299) / 400) * 86400;
}

// Column offsets
static constexpr int COL_RND = 10;
static constexpr int COL_NAME = 55;
static constexpr int COL_STATUS = 315;
static constexpr int COL_DATE = 470;

CalendarView::CalendarView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2) // RowH=46px, 5 rows fits 230px content area
{
}

void CalendarView::onEnter()
{
    nextRoundIdx = -1;
    time_t nowLocal = timeMgr->getLocalTime();

    // Find next round
    for (int i = 0; i < (int)cache->calendar.size(); i++)
    {
        const auto &rm = cache->calendar[i];
        
        time_t sessionLocal = 0;
        if (rm.sessionCount > 0) {
            struct tm st;
            strptime(rm.sessions[0].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
            sessionLocal = my_timegm(&st) + (timeMgr->getUTCOffset() * 3600);
        } else {
            struct tm rt;
            strptime(rm.date, "%Y-%m-%d", &rt);
            sessionLocal = mktime(&rt);
        }

        if (sessionLocal > nowLocal)
        {
            nextRoundIdx = i;
            break;
        }
    }

    // Auto-scroll logic: set cursor before fullRedraw (which calls updateScrollOffset)
    if (nextRoundIdx != -1) {
        _cursor = nextRoundIdx;
    } else {
        _cursor = 0;
    }

    ScrollListView::onEnter();
}

int CalendarView::dataSize() const
{
    return cache->calendar.size();
}

void CalendarView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("SEASON CALENDAR", 75, 12);

    // Red separator
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

    // Column headers
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("RND", COL_RND, 34);
    _tft->drawString("VENUE / COUNTRY", COL_NAME, 34);
    _tft->setTextDatum(top_right);
    _tft->drawString("STATUS", COL_STATUS, 34);
    _tft->drawString("DATE", COL_DATE, 34);
}

void CalendarView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= (int)cache->calendar.size()) return;
    const auto &rm = cache->calendar[dataIdx];
    bool isNext = (dataIdx == nextRoundIdx);

    float brightness = rowBrightness(dist);
    uint32_t textCol = selected ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);
    uint32_t dimTextCol = selected ? UI::COL_TEXT_DIM : dimCol(UI::COL_TEXT_DIM, brightness);

    // Selection/Next Background
    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    } else if (isNext) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, 0x1000); // Subtle red glow for next race
        _rowSprite->drawFastVLine(0, 0, _rowH, UI::COL_F1_RED);
    }

    // Round
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(selected ? UI::Fonts::DATA_ACCENT : UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(isNext ? UI::COL_F1_RED : textCol);
    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", rm.round);
    _rowSprite->drawString(roundStr, COL_RND, _rowH / 2);

    // Name
    _rowSprite->setFont(selected ? UI::Fonts::BODY_MAIN : UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(textCol);
    _rowSprite->drawString(rm.circuit.countryName, COL_NAME, _rowH / 2);

    // Status
    const char *status = "DONE";
    uint32_t statusCol = dimTextCol;
    
    if (isNext) {
        status = "NEXT UP";
        statusCol = UI::COL_F1_RED;
    } else if (dataIdx > nextRoundIdx && nextRoundIdx != -1) {
        status = "UPCOMING";
        statusCol = dimTextCol;
    } else if (nextRoundIdx == -1) {
        status = "UPCOMING";
    }

    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(selected ? statusCol : dimCol(statusCol, brightness));
    _rowSprite->drawString(status, COL_STATUS - 10, _rowH / 2);

    // Date
    _rowSprite->setTextColor(dimTextCol);
    _rowSprite->drawString(rm.date, COL_DATE, _rowH / 2);
}

void CalendarView::onPress()
{
    int idx = _cursor;
    if (idx >= 0 && idx < (int)cache->calendar.size())
    {
        _dm->launchWeekendView(&cache->calendar[idx]);
    }
}
