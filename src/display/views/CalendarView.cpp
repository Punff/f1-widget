#include "CalendarView.h"
#include "../../data/DataCache.h"

extern DataCache *cache;

CalendarView::CalendarView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _cursor(0), _scrollOffset(0) {}

void CalendarView::render()
{
    _fullRedraw();
}
void CalendarView::onEnter()
{
    _cursor = 0;
    _tft->fillScreen(TFT_BLACK);
    _updateScrollOffset();
    _fullRedraw();
}

void CalendarView::_updateScrollOffset()
{
    _scrollOffset = _cursor - CENTER_ROW;
}

void CalendarView::_renderHeader()
{
    _tft->fillRect(0, 0, SAFE_W, 40, 0x0000);
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(0xBDF7);
    _tft->drawString("2026 FORMULA 1", 10, 12);
    _tft->setTextColor(TFT_WHITE);
    _tft->drawString("SEASON CALENDAR", 10, 26);
    _tft->drawFastHLine(0, 39, SAFE_W, 0xE80020);
}

void CalendarView::_renderRow(int row, int idx, int rowY)
{
    bool selected = (row == CENTER_ROW);
    uint16_t bg = selected ? _tft->color24to16(0x1A1A1A) : 0x0000;

    if (idx < 0 || idx >= (int)cache->calendar.size())
    {
        _tft->fillRect(0, rowY, SAFE_W, ROW_H, TFT_BLACK);
        return;
    }

    const auto &rm = cache->calendar[idx];
    _tft->fillRect(0, rowY, SAFE_W, ROW_H, bg);

    // Fade logic
    int dist = abs(row - CENTER_ROW);
    float brightness = (dist == 0) ? 1.0f : (1.0f - (dist * 0.25f));
    uint16_t mainCol = selected ? TFT_WHITE : _tft->color24to16(0x7FFFFF); // Dimmed cyan/white

    // Round Number
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(0xE800, bg); // F1 Orange/Red for rounds

    _tft->setCursor(10, rowY + 12);
    _tft->printf("R%02d", rm.round);

    // Country/Location
    _tft->setTextColor(mainCol, bg);
    _tft->setTextSize(selected ? 2 : 1);
    _tft->drawString(rm.circuit.countryName, 60, rowY + ROW_H / 2);

    // Date
    _tft->setTextDatum(middle_right);
    _tft->setTextSize(1);
    _tft->setTextColor(0xBDF7, bg);
    _tft->drawString(rm.date, SAFE_W - 15, rowY + ROW_H / 2);
}

void CalendarView::_fullRedraw()
{
    if (cache->calendar.empty())
    {
        _tft->drawCenterString("LOADING SCHEDULE...", 200, 160);
        return;
    }
    _tft->startWrite();
    _renderHeader();
    for (int i = 0; i < ROWS_VISIBLE; i++)
    {
        _renderRow(i, _scrollOffset + i, 40 + (i * ROW_H));
    }
    _tft->endWrite();
}

void CalendarView::onTurnRight()
{
    if (_cursor < (int)cache->calendar.size() - 1)
    {
        _cursor++;
        _updateScrollOffset();
        _fullRedraw();
    }
}

void CalendarView::onTurnLeft()
{
    if (_cursor > 0)
    {
        _cursor--;
        _updateScrollOffset();
        _fullRedraw();
    }
}