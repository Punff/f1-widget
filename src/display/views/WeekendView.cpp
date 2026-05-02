#include "WeekendView.h"
#include "../../../include/UI_Fonts.h"
#include "../../time/TimeManager.h"
#include "../../data/DataCache.h"
#include "../DisplayManager.h"
#include <time.h>

extern TimeManager *timeMgr;

// Portable timegm replacement
static time_t my_timegm(struct tm *t) {
    return t->tm_sec + t->tm_min*60 + t->tm_hour*3600 + t->tm_yday*86400 +
           (t->tm_year-70)*31536000 + ((t->tm_year-69)/4)*86400 -
           ((t->tm_year-1)/100)*86400 + ((t->tm_year+299)/400)*86400;
}

WeekendView::WeekendView(LGFX *tft, DisplayManager *dm, const RaceMeeting *meeting)
    : ScrollListView(tft, dm, 40, 5, 2), _meeting(meeting) {}

int WeekendView::dataSize() const {
    return (_meeting && _meeting->sessionCount > 0) ? _meeting->sessionCount : 1;
}

void WeekendView::drawHeader() {
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);
    
    if (!_meeting) return;
    
    // Round number in red
    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", _meeting->round);
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(roundStr, 10, 8);
    
    // GP Name (shortened)
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    String gpName = String(_meeting->officialName);
    gpName.replace("Grand Prix", "GP");
    _tft->drawString(gpName.c_str(), 70, 12);
    
    // Circuit name
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(_meeting->circuit.shortName, 70, 30);
    
    // Separator
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);
}

void WeekendView::drawRow(int dataIdx, bool selected, int dist) {
    if (!_meeting) return;
    
    // If no sessions, show a placeholder
    if (_meeting->sessionCount == 0) {
        _rowSprite->fillSprite(selected ? UI::COL_BG_SEL : UI::COL_BG);
        _rowSprite->setTextDatum(middle_center);
        _rowSprite->setTextColor(UI::COL_MUTED);
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->drawString("No session data available", _rowSprite->width()/2, _rowH/2);
        return;
    }
    
    if (dataIdx < 0 || dataIdx >= _meeting->sessionCount) return;
    
    const Session &s = _meeting->sessions[dataIdx];
    
    // Check if this is the next upcoming session
    bool isNextSession = false;
    time_t nowLocal = timeMgr->getLocalTime();
    
    struct tm sessionTm;
    strptime(s.dateUtc, "%Y-%m-%dT%H:%M:%SZ", &sessionTm);
    time_t sessionUtc = my_timegm(&sessionTm);
    time_t sessionLocal = sessionUtc + (timeMgr->getUTCOffset() * 3600);
    
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
    
    // Right stripe for selected or next session
    if (selected || isNextSession) {
        _rowSprite->fillRect(UI::SAFE_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }
    
    // Day of week
    struct tm localTm;
    localtime_r(&sessionLocal, &localTm);
    char dayStr[4];
    strftime(dayStr, sizeof(dayStr), "%a", &localTm);
    
    // Time
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &localTm);
    
    // Draw
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(UI::COL_TEXT);
    _rowSprite->drawString(dayStr, 15, _rowH/2);
    _rowSprite->drawString(s.name, 70, _rowH/2);
    
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->drawString(timeStr, 420, _rowH/2);
    
    if (isNextSession) {
        _rowSprite->setTextColor(UI::COL_F1_RED);
        _rowSprite->drawString("▶", 460, _rowH/2);
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
