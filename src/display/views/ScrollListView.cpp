#include "ScrollListView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

ScrollListView::ScrollListView(LGFX *tft, DisplayManager *dm, int rowH, int rowsVisible, int centerRow)
    : _tft(tft), _dm(dm), _cursor(0), _scrollOffset(0),
      _rowH(rowH), _rowsVisible(rowsVisible), _centerRow(centerRow)
{
    _lastFooterText[0] = '\0';
    _rowSprite = nullptr; // Will get from DisplayManager
}

ScrollListView::~ScrollListView()
{
    // Do not delete shared sprite
}

void ScrollListView::onEnter()
{
    _rowSprite = _dm->rowSprite();
    if (_rowSprite) {
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->createSprite(UI::SCREEN_W, _rowH);
    }

    _tft->fillScreen(UI::COL_BG);
    fullRedraw();
}

void ScrollListView::onExit()
{
    // Nothing to do for shared sprite
}

void ScrollListView::render()
{
    // No full redraw needed - only redraw on input
}

void ScrollListView::onTurnRight()
{
    if (_cursor < dataSize() - 1)
    {
        int oldCursor = _cursor;
        _cursor++;
        partialRedraw(oldCursor);
    }
}

void ScrollListView::onTurnLeft()
{
    if (_cursor > 0)
    {
        int oldCursor = _cursor;
        _cursor--;
        partialRedraw(oldCursor);
    }
}

void ScrollListView::updateScrollOffset()
{
    // Keep selected row at center IF possible
    int newOffset = _cursor - _centerRow;
    
    // Clamp: don't go below 0
    if (newOffset < 0) newOffset = 0;
    
    // Clamp: don't exceed (dataSize - rowsVisible)
    int maxOffset = dataSize() - _rowsVisible;
    if (maxOffset < 0) maxOffset = 0;
    if (newOffset > maxOffset) newOffset = maxOffset;
    
    _scrollOffset = newOffset;
}

void ScrollListView::fullRedraw()
{
    updateScrollOffset();

    drawHeader();

    for (int row = 0; row < _rowsVisible; row++)
        drawSingleRow(row);

    _tft->drawFastHLine(0, UI::FOOTER_Y - 1, UI::SCREEN_W, UI::COL_F1_RED);
    drawFooter();
}

void ScrollListView::partialRedraw(int oldCursor)
{
    int oldOffset = _scrollOffset;
    updateScrollOffset();

    if (oldOffset != _scrollOffset)
    {
        for (int row = 0; row < _rowsVisible; row++)
            drawSingleRow(row);
        drawFooter();
        return;
    }

    int oldRow = oldCursor - _scrollOffset;
    if (oldRow >= 0 && oldRow < _rowsVisible)
        drawSingleRow(oldRow);

    int newRow = _cursor - _scrollOffset;
    if (newRow >= 0 && newRow < _rowsVisible)
        drawSingleRow(newRow);

    drawFooter();
}

void ScrollListView::drawSingleRow(int row)
{
    if (!_rowSprite || !_rowSprite->getBuffer())
        return;

    int dataIdx = _scrollOffset + row;
    bool selected = (dataIdx == _cursor);
    int dist = abs(dataIdx - _cursor);
    int rowY = UI::HEADER_H + (row * _rowH);

    // Clear sprite to black first
    _rowSprite->fillSprite(UI::COL_BG);

    // Draw row content
    if (dataIdx >= 0 && dataIdx < dataSize())
    {
        drawRow(dataIdx, selected, dist);
    }

    // Push row to screen
    _rowSprite->pushSprite(0, rowY);
}

float ScrollListView::rowBrightness(int dist) const
{
    if (dist <= 0)
        return 1.0f;
    float b = 1.0f - (dist * UI::DIM_RATE);
    return (b < UI::MIN_BRIGHT) ? UI::MIN_BRIGHT : b;
}

uint32_t ScrollListView::dimCol(uint32_t col, float brightness) const
{
    uint8_t r = ((col >> 16) & 0xFF) * brightness;
    uint8_t g = ((col >> 8) & 0xFF) * brightness;
    uint8_t b = (col & 0xFF) * brightness;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

bool ScrollListView::_footerTextChanged(const char *newText)
{
    if (strcmp(_lastFooterText, newText) == 0)
    {
        return false; // Same text, no change
    }
    strncpy(_lastFooterText, newText, sizeof(_lastFooterText) - 1);
    _lastFooterText[sizeof(_lastFooterText) - 1] = '\0';
    return true; // Text changed
}

void ScrollListView::drawFooterText(const char *text, int x, int y, uint32_t color, uint8_t size)
{
    // Clear footer area to prevent text overlap
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);

    _tft->setTextColor(color);
    _tft->setTextSize(size);
    _tft->setTextDatum(middle_right);
    _tft->drawString(text, x, y);

    strncpy(_lastFooterText, text, sizeof(_lastFooterText) - 1);
}
