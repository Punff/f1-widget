#include "ScrollListView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

ScrollListView::ScrollListView(LGFX *tft, DisplayManager *dm, int rowH, int rowsVisible, int centerRow)
    : _tft(tft), _dm(dm), _cursor(0), _scrollOffset(0),
      _rowH(rowH), _rowsVisible(rowsVisible), _centerRow(centerRow)
{
    _rowSprite = nullptr; // Will get from DisplayManager
}

ScrollListView::~ScrollListView()
{
    // Do not delete shared sprite
}

void ScrollListView::onEnter()
{
    _rowSprite = _dm->rowSprite();
    if (_rowSprite && !_rowSprite->getBuffer()) {
        Serial.println("[SPRITE] Buffer null at onEnter");
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


