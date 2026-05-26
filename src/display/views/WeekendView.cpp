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

        _sessionTimes.clear();
        int utcOffset = timeMgr->getUTCOffset() * 3600;
        for (int i = 0; i < _meeting->sessionCount; i++) {
            struct tm st;
            strptime(_meeting->sessions[i].dateUtc, "%Y-%m-%dT%H:%M:%SZ", &st);
            _sessionTimes.push_back(my_timegm(&st) + utcOffset);
        }
    } else {
        Serial.println("[WEEKEND] ERROR: Meeting pointer is null!");
    }
    ScrollListView::onEnter();
}

int WeekendView::dataSize() const {
    return (_meeting && _meeting->sessionCount > 0) ? _meeting->sessionCount : 1;
}

void WeekendView::drawHeader() {
    if (!_meeting) return;

    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", _meeting->round);
    _dm->header()->draw(_meeting->officialName, _meeting->circuit.shortName, roundStr);
}

static const char *sessionAbbrev(const char *name) {
    if (strcmp(name, "Practice 1") == 0) return "FP1";
    if (strcmp(name, "Practice 2") == 0) return "FP2";
    if (strcmp(name, "Practice 3") == 0) return "FP3";
    if (strcmp(name, "Qualifying") == 0) return "Q";
    if (strcmp(name, "Sprint") == 0) return "S";
    if (strcmp(name, "Sprint Qualifying") == 0) return "SQ";
    if (strcmp(name, "Race") == 0) return "R";
    return name;
}

void WeekendView::drawRow(int dataIdx, bool selected, int dist) {
    if (!_meeting) return;

    if (_meeting->sessionCount == 0) {
        _rowSprite->setTextDatum(middle_center);
        _rowSprite->setTextColor(dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->drawString("No session data", UI::SCREEN_W/2, _rowH/2);
        return;
    }

    if (dataIdx < 0 || dataIdx >= _meeting->sessionCount) return;

    const Session &s = _meeting->sessions[dataIdx];
    time_t nowLocal = timeMgr->getLocalTime();
    time_t sessionLocal = (dataIdx < (int)_sessionTimes.size()) ? _sessionTimes[dataIdx] : 0;

    int nextIdx = -1;
    for (int i = 0; i < _meeting->sessionCount; i++) {
        time_t sl = (i < (int)_sessionTimes.size()) ? _sessionTimes[i] : 0;
        if (sl > nowLocal) { nextIdx = i; break; }
    }

    bool isPast = (nextIdx == -1 || dataIdx < nextIdx);
    bool isNext = (dataIdx == nextIdx);
    bool isFuture = (!isPast && !isNext);

    uint32_t accentCol;
    uint32_t textCol;

    if (selected) {
        accentCol = UI::COL_F1_RED;
        textCol = UI::COL_TEXT;
    } else if (isNext) {
        accentCol = UI::COL_F1_RED;
        textCol = UI::COL_F1_RED;
    } else if (isFuture) {
        accentCol = UI::COL_TEXT_DIM;
        textCol = (dist < 2) ? UI::COL_TEXT_DIM : UI::COL_MUTED;
    } else {
        accentCol = 0;
        textCol = UI::COL_MUTED;
    }

    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    } else if (isNext) {
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
    } else if (isFuture) {
        _rowSprite->fillRect(0, 0, 2, _rowH, 0x444444);
    }

    struct tm localTm;
    localtime_r(&sessionLocal, &localTm);
    char dayStr[8];
    strftime(dayStr, sizeof(dayStr), "%a", &localTm);
    char dateStr[4];
    strftime(dateStr, sizeof(dateStr), "%d", &localTm);
    char timeStr[12];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &localTm);

    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(isNext ? UI::COL_F1_RED : textCol);
    _rowSprite->drawString(dayStr, COL_DAY, _rowH / 2 - 6);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(textCol);
    _rowSprite->drawString(dateStr, COL_DAY, _rowH / 2 + 6);

    _rowSprite->setFont(UI::Fonts::DATA_ACCENT);
    _rowSprite->setTextColor(isNext ? UI::COL_F1_RED : textCol);
    _rowSprite->drawString(sessionAbbrev(s.name), COL_NAME, _rowH / 2);

    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(textCol);
    _rowSprite->drawString(timeStr, COL_TIME, _rowH / 2);

    if (isNext) {
        _rowSprite->setTextColor(UI::COL_F1_RED);
        _rowSprite->drawString("NEXT", COL_NEXT, _rowH / 2);
    }
}

void WeekendView::drawFooter() {
    _dm->footer()->draw();

    if (!_meeting) {
        _dm->footer()->drawCenter("", 0);
        return;
    }

    time_t nowLocal = timeMgr->getLocalTime();
    time_t nextSessionTime = 0;
    const char *nextSessionName = nullptr;

    for (int i = 0; i < _meeting->sessionCount; i++) {
        time_t sl = (i < (int)_sessionTimes.size()) ? _sessionTimes[i] : 0;
        if (sl > nowLocal) {
            nextSessionTime = sl;
            nextSessionName = _meeting->sessions[i].name;
            break;
        }
    }

    if (nextSessionName) {
        time_t diff = nextSessionTime - nowLocal;
        char cd[24];
        formatCountdown(cd, sizeof(cd), diff);
        char buf[48];
        snprintf(buf, sizeof(buf), "WK \xc2\xb7 %s in %s", nextSessionName, cd);
        _dm->footer()->drawCenter(buf, UI::COL_MUTED);
    } else {
        _dm->footer()->drawCenter("WK \xc2\xb7 Complete", UI::COL_MUTED);
    }
    _lastFooterSec = nowLocal;
}

void WeekendView::tick() {
    time_t now = timeMgr->getLocalTime();
    if (now == _lastFooterSec) return;
    _lastFooterSec = now;

    if (!_meeting) return;

    // Find next upcoming session and update center text only
    time_t nextSessionTime = 0;
    const char *nextSessionName = nullptr;

    for (int i = 0; i < _meeting->sessionCount; i++) {
        time_t sl = (i < (int)_sessionTimes.size()) ? _sessionTimes[i] : 0;
        if (sl > now) {
            nextSessionTime = sl;
            nextSessionName = _meeting->sessions[i].name;
            break;
        }
    }

    if (nextSessionName) {
        time_t diff = nextSessionTime - now;
        char cd[24];
        formatCountdown(cd, sizeof(cd), diff);
        char buf[48];
        snprintf(buf, sizeof(buf), "WK \xc2\xb7 %s in %s", nextSessionName, cd);
        _dm->footer()->drawCenter(buf, UI::COL_MUTED);
    } else {
        _dm->footer()->drawCenter("WK \xc2\xb7 Complete", UI::COL_MUTED);
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
    _dm->returnToCalendar();
}
