#include "CalendarView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"
#include "../../time/TimeManager.h"
#include "../../time/TimeUtils.h"
#include "../DisplayManager.h"
#include <time.h>

extern DataCache *cache;
extern TimeManager *timeMgr;

// Status: 0=done, 1=live, 2=next, 3=future
static constexpr int ST_DONE = 0;
static constexpr int ST_LIVE = 1;
static constexpr int ST_NEXT = 2;
static constexpr int ST_FUTURE = 3;

CalendarView::CalendarView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 43, 5, 2, 14), _lastFooterSec(0), _nextRoundIdx(-1)
{
}

void CalendarView::onEnter()
{
    _nextRoundIdx = -1;
    _nextRoundTime = 0;
    _sessionTimes.clear();
    _status.clear();

    time_t nowLocal = timeMgr->getLocalTime();
    struct tm nowTm;
    localtime_r(&nowLocal, &nowTm);
    char todayStr[16];
    strftime(todayStr, sizeof(todayStr), "%Y-%m-%d", &nowTm);

    int utcOffset = timeMgr->getUTCOffset() * 3600;
    int nextFound = -1;

    _status.reserve(cache->calendar.size());
    _sessionTimes.reserve(cache->calendar.size());

    for (int i = 0; i < (int)cache->calendar.size(); i++)
    {
        const auto &rm = cache->calendar[i];

        // Compute session[0] local time for footer countdown
        time_t sessionLocal = 0;
        if (rm.sessionCount > 0) {
            struct tm st = {0};
            if (strptime(rm.sessions[0].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st))
                sessionLocal = my_timegm(&st) + utcOffset;
        }
        _sessionTimes.push_back(sessionLocal);

        // Determine status via date comparison
        char sd[16] = "";
        if (rm.sessionCount > 0)
            strncpy(sd, rm.sessions[0].dateUtc, 10);

        int st;
        if (sd[0] && strcmp(todayStr, sd) >= 0 && strcmp(todayStr, rm.date) <= 0) {
            st = ST_LIVE;
        } else if (sd[0] && strcmp(todayStr, sd) < 0) {
            if (nextFound < 0) {
                st = ST_NEXT;
                nextFound = i;
            } else {
                st = ST_FUTURE;
            }
        } else {
            st = ST_DONE;
        }
        _status.push_back(st);

        // Track next future weekend for footer countdown
        if (nextFound == i) {
            _nextRoundIdx = i;
            _nextRoundTime = sessionLocal;
        }
    }

    // Cursor: live weekend first, else next, else last event (offseason)
    _cursor = 0;
    for (int i = 0; i < (int)_status.size(); i++) {
        if (_status[i] == ST_LIVE) { _cursor = i; break; }
    }
    if (_cursor == 0) {
        for (int i = 0; i < (int)_status.size(); i++) {
            if (_status[i] == ST_NEXT) { _cursor = i; break; }
        }
    }
    if (_cursor == 0 && !_status.empty()) {
        _cursor = _status.size() - 1;
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

    int chY = UI::HEADER_H + _colH / 2;
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("RND", UI::COL_POS, chY);
    _tft->drawString("VENUE", UI::COL_PRIMARY, chY);

    _tft->setTextDatum(middle_right);
    _tft->drawString("DATE", UI::COL_VALUE_R, chY);
    _tft->drawString("STATUS", UI::COL_END_R, chY);
}

void CalendarView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= (int)cache->calendar.size()) return;
    const auto &rm = cache->calendar[dataIdx];
    int st = (dataIdx < (int)_status.size()) ? _status[dataIdx] : ST_DONE;
    bool isNext = (st == ST_NEXT);
    bool isLive = (st == ST_LIVE);

    uint32_t textCol = selected ? UI::COL_TEXT :
                        (isNext ? UI::COL_ACCENT :
                         (isLive ? UI::COL_ACCENT :
                          (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED)));

    // Background and accent bar (matching WeekendView style)
    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_ACCENT);
    } else if (isNext) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_NEXT_BG);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
    } else if (isLive) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_NEXT_BG);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
    } else if (st == ST_FUTURE) {
        _rowSprite->fillRect(0, 0, 2, _rowH, 0x444444);
    }

    // Round number
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(isNext || isLive ? UI::COL_ACCENT : textCol);
    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", rm.round);
    _rowSprite->drawString(roundStr, UI::COL_POS, _rowH / 2);

    // Venue (country)
    _rowSprite->setTextColor(textCol);
    _rowSprite->drawString(rm.circuit.countryName, UI::COL_PRIMARY, _rowH / 2);

    // Date (right-aligned)
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(textCol);
    // Show MM-DD from rm.date instead of full YYYY-MM-DD
    _rowSprite->drawString(rm.date + 5, UI::COL_VALUE_R, _rowH / 2);

    // Status label (right-aligned, far right)
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    if (isNext) {
        _rowSprite->setTextColor(UI::COL_ACCENT);
        _rowSprite->drawString("NEXT", UI::COL_END_R, _rowH / 2);
    } else if (isLive) {
        _rowSprite->setTextColor(UI::COL_ACCENT);
        _rowSprite->drawString("LIVE", UI::COL_END_R, _rowH / 2);
    }
}

void CalendarView::onPress()
{
    int idx = _cursor;
    if (idx >= 0 && idx < (int)cache->calendar.size())
    {
        int rowY = UI::HEADER_H + _colH + (_cursor - _scrollOffset) * _rowH;
        _tft->fillRect(0, rowY, UI::SCREEN_W, _rowH, UI::COL_BG_SEL);
        delay(40);
        _dm->launchWeekendView(&cache->calendar[idx]);
    }
}

void CalendarView::drawFooter()
{
    _dm->footer()->draw();

    if (_nextRoundIdx < 0 || _nextRoundIdx >= (int)cache->calendar.size()) return;

    const auto &next = cache->calendar[_nextRoundIdx];
    time_t nowLocal = timeMgr->getLocalTime();
    time_t diff = _nextRoundTime - nowLocal;
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

        if (_nextRoundIdx < 0 || _nextRoundIdx >= (int)cache->calendar.size()) return;
        const auto &next = cache->calendar[_nextRoundIdx];
        time_t diff = _nextRoundTime - now;
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
