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

CalendarView::CalendarView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 44, 5, 2) // RowH=44px, 5 rows, center at index 2
{
}

void CalendarView::onEnter()
{
    ScrollListView::onEnter();
    nextRoundIdx = -1;
    time_t nowLocal = timeMgr->getLocalTime();

    // Find next round (first round with sessions in the future)
    for (int i = 0; i < cache->calendar.size(); i++)
    {
        const auto &rm = cache->calendar[i];
        if (rm.sessionCount == 0)
            continue;

        struct tm st;
        strptime(rm.sessions[0].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
        time_t sessionLocal = my_timegm(&st) + (timeMgr->getUTCOffset() * 3600);

        if (sessionLocal > nowLocal)
        {
            nextRoundIdx = i;
            break;
        }
    }

    // If no sessions found, try using the race date
    if (nextRoundIdx == -1)
    {
        for (int i = 0; i < cache->calendar.size(); i++)
        {
            const auto &rm = cache->calendar[i];
            struct tm rt;
            strptime(rm.date, "%Y-%m-%d", &rt);
            time_t raceLocal = mktime(&rt);

            if (raceLocal > nowLocal)
            {
                nextRoundIdx = i;
                break;
            }
        }
    }
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
    _tft->drawString("RND", 15, 38);
    _tft->drawString("COUNTRY", 70, 38);
    _tft->drawString("DATE", 420, 38);
}

void CalendarView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= cache->calendar.size())
        return;

    const auto &rm = cache->calendar[dataIdx];
    bool isNext = (dataIdx == nextRoundIdx);

    // Right-side stripe for selected or NEXT round
    if (selected || isNext)
    {
        _rowSprite->fillRect(UI::SAFE_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }

    // Calculate brightness for non-selected rows
    float brightness = rowBrightness(dist);
    uint32_t textCol = (selected || isNext) ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);

    // Round number
    _rowSprite->setTextDatum(middle_left);
    if (selected || isNext)
    {
        _rowSprite->setFont(UI::Fonts::DATA_ACCENT);
        _rowSprite->setTextColor(UI::COL_F1_RED);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
    }

    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", rm.round);
    _rowSprite->drawString(roundStr, 15, _rowH / 2);

    // Country name
    _rowSprite->setTextDatum(middle_left);
    if (selected || isNext)
    {
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(UI::COL_TEXT);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
    }
    _rowSprite->drawString(rm.circuit.countryName, 70, _rowH / 2);

    // Compute status
    const char *status = "UPCOMING";
    if (isNext)
    {
        status = "NEXT";
    }
    else
    {
        bool isDone = true;

        if (rm.sessionCount > 0)
        {
            time_t nowLocal = timeMgr->getLocalTime();
            for (int i = 0; i < rm.sessionCount; i++)
            {
                struct tm st;
                strptime(rm.sessions[i].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
                time_t sessionLocal = my_timegm(&st) + (timeMgr->getUTCOffset() * 3600);
                if (sessionLocal > nowLocal)
                {
                    isDone = false;
                    break;
                }
            }
        }
        else
        {
            struct tm rt;
            strptime(rm.date, "%Y-%m-%d", &rt);
            time_t raceLocal = mktime(&rt);
            if (raceLocal > timeMgr->getLocalTime())
                isDone = false;
        }

        if (isDone)
            status = "DONE";
    }

    // Status tag (left of date)
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    uint32_t statusCol = UI::COL_MUTED;
    if (strcmp(status, "NEXT") == 0)
        statusCol = UI::COL_F1_RED;
    else if (strcmp(status, "DONE") == 0)
        statusCol = UI::COL_TEXT_DIM;
    _rowSprite->setTextColor(statusCol);
    _rowSprite->drawString(status, 400, _rowH / 2);

    // Date (right-aligned)
    _rowSprite->setTextColor(UI::COL_TEXT_DIM);
    _rowSprite->drawString(rm.date, 460, _rowH / 2);
}

void CalendarView::onPress()
{
    int idx = _cursor;
    if (idx >= 0 && idx < cache->calendar.size())
    {
        _dm->launchWeekendView(&cache->calendar[idx]);
    }
}
