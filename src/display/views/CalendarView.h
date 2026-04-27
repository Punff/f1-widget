#pragma once
#include "../IView.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager;

class CalendarView : public IView
{
public:
    CalendarView(LGFX *tft, DisplayManager *dm);

    void onEnter() override;
    void onExit() override {}
    void render() override;
    void tick() override {}

    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override {}
    void onLongPress() override {}
    void onDoublePress() override {}

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _cursor;
    int _scrollOffset;

    void _updateScrollOffset();
    void _renderHeader();
    void _renderRow(int row, int idx, int rowY);
    void _fullRedraw();

    static constexpr int ROW_H = 40; // Slightly taller for dates
    static constexpr int ROWS_VISIBLE = 6;
    static constexpr int CENTER_ROW = 2;
    static constexpr int SAFE_W = 390;
};