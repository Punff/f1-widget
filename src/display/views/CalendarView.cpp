#include "CalendarView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

CalendarView::CalendarView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 44, 6, 2)  // RowH=44px, 6 rows, center at index 2
{
}

int CalendarView::dataSize() const
{
    return cache->calendar.size();
}

void CalendarView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title - use centralized fonts
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
    const auto &rm = cache->calendar[dataIdx];

    // Right-side stripe for selected row
    if (selected)
    {
        _rowSprite->fillRect(UI::SAFE_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }

    // Calculate brightness
    float brightness = rowBrightness(dist);
    uint32_t textCol = selected ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);

    // Round number
    _rowSprite->setTextDatum(middle_left);
    if (selected)
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
    if (selected)
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

    // Date (right-aligned)
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(UI::COL_TEXT_DIM);
    _rowSprite->drawString(rm.date, 460, _rowH / 2);
}
