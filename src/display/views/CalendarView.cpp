#include "CalendarView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"
#include "../../time/TimeManager.h"
#include "../../time/TimeUtils.h"
#include "../DisplayManager.h"

extern DataCache *cache;
extern TimeManager *timeMgr;
static int nextRoundIdx = -1;

// Column offsets
static constexpr int COL_RND = 12;
static constexpr int COL_NAME = 75;
static constexpr int COL_DATE = 360;
static constexpr int COL_STATUS = 468;

CalendarView::CalendarView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2), _lastFooterSec(0)
{
}

void CalendarView::onEnter()
{
    nextRoundIdx = -1;
    time_t nowLocal = timeMgr->getLocalTime();

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
    _dm->header()->draw("SEASON CALENDAR");

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("RND", COL_RND, 34);
    _tft->drawString("VENUE", COL_NAME, 34);
    _tft->setTextDatum(top_right);
    _tft->drawString("DATE", COL_DATE, 34);
    _tft->drawString("STATUS", COL_STATUS, 34);
}

void CalendarView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= (int)cache->calendar.size()) return;
    const auto &rm = cache->calendar[dataIdx];
    bool isNext = (dataIdx == nextRoundIdx);
    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    } else if (isNext) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, 0x1000);
        _rowSprite->drawFastVLine(0, 0, _rowH, UI::COL_F1_RED);
    }

    // Round
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(isNext ? UI::COL_F1_RED : dim);
    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", rm.round);
    _rowSprite->drawString(roundStr, COL_RND, _rowH / 2);

    // Venue / country
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(rm.circuit.countryName, COL_NAME, _rowH / 2);

    // Status
    uint32_t statusCol = dim;

    if (isNext) {
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(UI::COL_F1_RED);
        _rowSprite->drawString("NEXT", COL_STATUS, _rowH / 2);
    } else if (dataIdx > nextRoundIdx && nextRoundIdx != -1) {
        _rowSprite->setTextDatum(middle_right);
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(dim);
        _rowSprite->drawString("UPCOMING", COL_STATUS, _rowH / 2);
    }

    // Date
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(rm.date, COL_DATE, _rowH / 2);
}

void CalendarView::onPress()
{
    int idx = _cursor;
    if (idx >= 0 && idx < (int)cache->calendar.size())
    {
        // Brief flash on pressed row before transitioning
        int rowY = UI::HEADER_H + (_cursor - _scrollOffset) * _rowH;
        _tft->fillRect(0, rowY, UI::SCREEN_W, _rowH, UI::COL_BG_SEL);
        delay(40);
        _dm->launchWeekendView(&cache->calendar[idx]);
    }
}

void CalendarView::drawFooter()
{
    _dm->footer()->draw();

    if (nextRoundIdx < 0 || nextRoundIdx >= (int)cache->calendar.size()) return;

    const auto &next = cache->calendar[nextRoundIdx];
    time_t nowLocal = timeMgr->getLocalTime();
    time_t sessionLocal = 0;
    if (next.sessionCount > 0) {
        struct tm st;
        strptime(next.sessions[0].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
        sessionLocal = my_timegm(&st) + (timeMgr->getUTCOffset() * 3600);
    } else {
        struct tm rt;
        strptime(next.date, "%Y-%m-%d", &rt);
        sessionLocal = mktime(&rt);
    }

    time_t diff = sessionLocal - nowLocal;
    if (diff < 0) diff = 0;

    char buf[40];
    char cd[24];
    formatCountdown(cd, sizeof(cd), diff);
    snprintf(buf, sizeof(buf), "CAL \xc2\xb7 %s in %s", next.circuit.countryName, cd);
    _dm->footer()->drawCenter(buf, UI::COL_MUTED);
}

void CalendarView::tick()
{
    time_t now = timeMgr->getLocalTime();
    if (now != _lastFooterSec) {
        _lastFooterSec = now;

        if (nextRoundIdx < 0 || nextRoundIdx >= (int)cache->calendar.size()) return;
        const auto &next = cache->calendar[nextRoundIdx];
        time_t sessionLocal = 0;
        if (next.sessionCount > 0) {
            struct tm st;
            strptime(next.sessions[0].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
            sessionLocal = my_timegm(&st) + (timeMgr->getUTCOffset() * 3600);
        } else {
            struct tm rt;
            strptime(next.date, "%Y-%m-%d", &rt);
            sessionLocal = mktime(&rt);
        }
        time_t diff = sessionLocal - now;
        if (diff < 0) diff = 0;
        char buf[40];
        char cd[24];
        formatCountdown(cd, sizeof(cd), diff);
        snprintf(buf, sizeof(buf), "CAL \xc2\xb7 %s in %s", next.circuit.countryName, cd);
        _dm->footer()->drawCenter(buf, UI::COL_MUTED);
    }
}

void CalendarView::onLongPress()
{
    _dm->returnToMenu();
}
