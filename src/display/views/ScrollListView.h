#pragma once
#include "../IView.h"
#include "UI.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager;

class ScrollListView : public IView
{
public:
    ScrollListView(LGFX *tft, DisplayManager *dm, int rowH, int rowsVisible, int centerRow, int colH = 0);
    virtual ~ScrollListView();

    // Lifecycle
    void onEnter() override;
    void onExit() override;
    void render() override;
    void tick() override;

    // Input
    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override {}

protected:
    // Subclasses must implement these
    virtual int dataSize() const = 0;
    virtual void drawHeader() = 0;
    virtual void drawRow(int dataIdx, bool selected, int dist) = 0;
    virtual void drawFooter() {}

    // State
    int _cursor;
    int _scrollOffset;
    int _rowH;
    int _rowsVisible;
    int _centerRow;
    int _colH;

    // Draw helpers
    void updateScrollOffset();
    void fullRedraw();
    void partialRedraw(int oldCursor);
    void drawSingleRow(int row);

    // Animations
    void flashRow();
    int _shimmerSteps;

    // LGFX members
    LGFX *_tft;
    DisplayManager *_dm;
    LGFX_Sprite *_rowSprite;   // Sprite buffer for each row
};
