#include "WeekendView.h"
#include "../../../include/UI_Fonts.h"
#include "../../time/TimeManager.h"
#include "../../time/TimeUtils.h"
#include "../../data/DataCache.h"
#include "../DisplayManager.h"
#include <time.h>

extern TimeManager *timeMgr;

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

    if (!_meeting) {
        _dm->header()->redrawEncoder();
        return;
    }

    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", _meeting->round);
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(roundStr, 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    char nameBuf[48];
    strlcpy(nameBuf, _meeting->officialName, sizeof(nameBuf));
    _tft->drawString(nameBuf, 95, 12);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(_meeting->circuit.shortName, 95, 32);

    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

    _dm->header()->redrawEncoder();
}

void WeekendView::drawRow(int dataIdx, bool selected, int dist) {
    if (!_meeting) return;

    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    // If no sessions, show a placeholder
    if (_meeting->sessionCount == 0) {
        _rowSprite->setTextDatum(middle_center);
        _rowSprite->setTextColor(dim);
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->drawString("No session data", UI::SCREEN_W/2, _rowH/2);
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
    _rowSprite->setTextColor(isNextSession ? UI::COL_F1_RED : dim);
    _rowSprite->drawString(dayStr, COL_DAY, _rowH / 2 - 6);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(dateStr, COL_DAY, _rowH / 2 + 6);

    // Session name
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(s.name, COL_NAME, _rowH / 2);

    // Time
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(timeStr, COL_TIME, _rowH / 2);

    if (isNextSession) {
        _rowSprite->setTextColor(UI::COL_F1_RED);
        _rowSprite->drawString("NEXT", COL_NEXT, _rowH / 2);
    }
}

void WeekendView::drawFooter() {
    _dm->footer()->draw();

    char footerStr[48];
    time_t t = timeMgr->getLocalTime();
    struct tm lt;
    localtime_r(&t, &lt);
    char timeBuf[12];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &lt);
    snprintf(footerStr, sizeof(footerStr), "%s  UTC%+d", timeBuf, timeMgr->getUTCOffset());
    _dm->footer()->drawText(footerStr);
    _lastFooterSec = t;
}

void WeekendView::tick() {
    time_t now = timeMgr->getLocalTime();
    if (now != _lastFooterSec) {
        drawFooter();
    }
}

void WeekendView::onPress() {
    if (!_meeting || _meeting->sessionCount == 0) return;
    if (_cursor < 0 || _cursor >= _meeting->sessionCount) return;

    const Session &s = _meeting->sessions[_cursor];
    const char *name = s.name;

    // Only Race, Qualifying, and Sprint have structured results
    if (strcmp(name, "Race") == 0 || strcmp(name, "Qualifying") == 0 || strcmp(name, "Sprint") == 0)
    {
        int rowY = UI::HEADER_H + (_cursor - _scrollOffset) * _rowH;
        _tft->fillRect(0, rowY, UI::SCREEN_W, _rowH, UI::COL_BG_SEL);
        delay(40);
        _dm->launchSessionResultsView(_meeting, _cursor);
    }
}

void WeekendView::onLongPress() {
    _dm->returnToMenu();
}
