#include "ScrollListView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

ScrollListView::ScrollListView(LGFX *tft, DisplayManager *dm, int rowH, int rowsVisible, int centerRow)
    : _tft(tft), _dm(dm), _cursor(0), _scrollOffset(0),
      _rowH(rowH), _rowsVisible(rowsVisible), _centerRow(centerRow)
{
    _rowSprite = new LGFX_Sprite(_tft);
    _rowSprite->createSprite(UI::SAFE_W, _rowH);
    _lastFooterText[0] = '\0'; // Initialize empty

    // Set the "Global" default font for all list items here
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
}

ScrollListView::~ScrollListView()
{
    if (_rowSprite)
    {
        _rowSprite->deleteSprite();
        delete _rowSprite;
    }
}

void ScrollListView::onEnter()
{
    _cursor = 0;
    _tft->fillScreen(UI::COL_BG);
    fullRedraw();
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
    _scrollOffset = _cursor - _centerRow;
}

void ScrollListView::fullRedraw()
{
    updateScrollOffset();

    _tft->startWrite();

    // Draw header
    drawHeader();

    // Draw all rows
    for (int row = 0; row < _rowsVisible; row++)
    {
        drawSingleRow(row);
    }

    // Red separator line above footer
    _tft->drawFastHLine(0, UI::FOOTER_Y - 1, UI::SCREEN_W, UI::COL_F1_RED);

    // Draw footer
    drawFooter();

    _tft->endWrite();
}

void ScrollListView::partialRedraw(int oldCursor)
{
    // Calculate new scroll offset
    int newScrollOffset = _cursor - _centerRow;

    // Check if scroll offset changed (data shifted)
    if (newScrollOffset != _scrollOffset)
    {
        // Data shifted - redraw only rows and footer, NOT header
        _scrollOffset = newScrollOffset;

        _tft->startWrite();

        // Redraw all rows (data shifted)
        for (int row = 0; row < _rowsVisible; row++)
        {
            drawSingleRow(row);
        }

        // Redraw footer
        drawFooter();

        _tft->endWrite();
        return;
    }

    // Scroll offset didn't change - only selection moved
    // Calculate which rows changed: old selected and new selected
    int oldSelectedRow = _centerRow + (oldCursor - _scrollOffset);
    int newSelectedRow = _centerRow; // New cursor is at center

    _tft->startWrite();

    // Redraw old selected row (now unselected)
    if (oldSelectedRow >= 0 && oldSelectedRow < _rowsVisible)
    {
        drawSingleRow(oldSelectedRow);
    }

    // Redraw new selected row (now selected)
    drawSingleRow(newSelectedRow);

    // UPDATE FOOTER (selection changed)
    drawFooter();

    _tft->endWrite();

    // Draw footer
    drawFooter();

    _tft->endWrite();
}

void ScrollListView::drawSingleRow(int row)
{
    int dataIdx = _scrollOffset + row;
    bool selected = (row == _centerRow);
    int dist = abs(row - _centerRow);
    int rowY = UI::HEADER_H + (row * _rowH);

    // Background
    _rowSprite->fillSprite(selected ? UI::COL_BG_SEL : UI::COL_BG);

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
    // Always redraw footer - clear ENTIRE footer area
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);

    // Draw new text
    _tft->setTextColor(color);
    _tft->setTextSize(size);
    _tft->setTextDatum(middle_right);
    _tft->drawString(text, x, y);

    // Update last text for potential future optimization
    strncpy(_lastFooterText, text, sizeof(_lastFooterText) - 1);
    _lastFooterText[sizeof(_lastFooterText) - 1] = '\0';
}
