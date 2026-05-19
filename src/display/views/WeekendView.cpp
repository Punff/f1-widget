#include "WeekendView.h"
#include "../../../include/UI_Fonts.h"
#include "../../time/TimeManager.h"
#include "../../data/DataCache.h"
#include "../DisplayManager.h"
#include <time.h>

extern TimeManager *timeMgr;

static time_t my_timegm(struct tm *t) {
    return t->tm_sec + t->tm_min*60 + t->tm_hour*3600 + t->tm_yday*86400 +
           (t->tm_year-70)*31536000 + ((t->tm_year-69)/4)*86400 -
           ((t->tm_year-1)/100)*86400 + ((t->tm_year+299)/400)*86400;
}

static constexpr int COL_DAY = 15;
static constexpr int COL_NAME = 75;
static constexpr int COL_TIME = 365;
static constexpr int COL_NEXT = 468;

WeekendView::WeekendView(LGFX *tft, DisplayManager *dm, const RaceMeeting *meeting)
    : ScrollListView(tft, dm, 46, 5, 2), _meeting(meeting) {}

void WeekendView::onEnter() {
    if (_meeting) {
        Serial.printf("[WEEKEND] Entering view for %s (Key: %d, Sessions: %d)\n",
            _meeting->officialName, _meeting->meetingKey, _meeting->sessionCount);
    } else {
        Serial.println("[WEEKEND] ERROR: Meeting pointer is null!");
    }
    ScrollListView::onEnter();
}

int WeekendView::dataSize() const {
    return (_meeting && _meeting->sessionCount > 0) ? _meeting->sessionCount : 1;
}

void WeekendView::drawHeader() {
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    if (!_meeting) return;

    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", _meeting->round);
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(roundStr, 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(_meeting->officialName, 70, 12);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(_meeting->circuit.shortName, 70, 32);

    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);
}

void WeekendView::drawRow(int dataIdx, bool selected, int dist) {
    (void)dist;
    if (!_meeting) return;

    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }

    // If no sessions, show a placeholder
    if (_meeting->sessionCount == 0) {
        _rowSprite->setTextDatum(middle_center);
        _rowSprite->setTextColor(UI::COL_MUTED);
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->drawString("No session data available", UI::SCREEN_W/2, _rowH/2);
        return;
    }

    if (dataIdx < 0 || dataIdx >= _meeting->sessionCount) return;

    const Session &s = _meeting->sessions[dataIdx];

    // Check if this is the next upcoming session
    bool isNextSession = false;
    time_t nowLocal = timeMgr->getLocalTime();

    struct tm sessionTm;
    strptime(s.dateUtc, "%Y-%m-%dT%H:%M:%SZ", &sessionTm);
    time_t sessionLocal = my_timegm(&sessionTm) + (timeMgr->getUTCOffset() * 3600);

    if (sessionLocal > nowLocal) {
        for (int i = 0; i < _meeting->sessionCount; i++) {
            struct tm st;
            strptime(_meeting->sessions[i].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
            time_t sl = my_timegm(&st) + (timeMgr->getUTCOffset() * 3600);
            if (sl > nowLocal) {
                isNextSession = (i == dataIdx);
                break;
            }
        }
    }

    // Day of week
    struct tm localTm;
    localtime_r(&sessionLocal, &localTm);
    char dayStr[8];
    strftime(dayStr, sizeof(dayStr), "%a", &localTm);
    char dateStr[4];
    strftime(dateStr, sizeof(dateStr), "%d", &localTm);

    // Time
    char timeStr[12];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &localTm);

    // Day
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(isNextSession ? UI::COL_F1_RED : UI::COL_TEXT);
    _rowSprite->drawString(dayStr, COL_DAY, _rowH / 2 - 6);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(UI::COL_TEXT_DIM);
    _rowSprite->drawString(dateStr, COL_DAY, _rowH / 2 + 6);

    // Session name
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(UI::COL_TEXT);
    _rowSprite->drawString(s.name, COL_NAME, _rowH / 2);

    // Time
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setTextColor(UI::COL_TEXT);
    _rowSprite->drawString(timeStr, COL_TIME, _rowH / 2);

    if (isNextSession) {
        _rowSprite->setTextColor(UI::COL_F1_RED);
        _rowSprite->drawString("NEXT", COL_NEXT, _rowH / 2);
    }
}

void WeekendView::drawFooter() {
    _tft->fillRect(0, UI::SCREEN_H - UI::FOOTER_H, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);
    _tft->drawFastHLine(0, UI::SCREEN_H - UI::FOOTER_H, UI::SCREEN_W, UI::COL_F1_RED);

    char footerStr[32];
    snprintf(footerStr, sizeof(footerStr), "LOCAL TIME  UTC%+d", timeMgr->getUTCOffset());
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(footerStr, UI::SCREEN_W/2, UI::SCREEN_H - UI::FOOTER_H/2);
}

void WeekendView::onLongPress() {
    _dm->returnToMenu();
}
