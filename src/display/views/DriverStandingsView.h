#pragma once
#include "../IView.h"
#include "../../data/DataCache.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager; // Forward declaration

class DriverStandingsView : public IView
{
public:
    DriverStandingsView(LGFX *tft, DisplayManager *dm);

    // IView Interface Implementation
    void onEnter() override;
    void onExit() override {} // FIX: Added missing virtual function
    void render() override;

    // Input Handling
    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override {}       // FIX: Added missing virtual function
    void onLongPress() override {}   // FIX: Added missing virtual function
    void onDoublePress() override {} // FIX: Added missing virtual function

    // UI Constants
    static constexpr int HEADER_H = 32;
    static constexpr int FOOTER_H = 24;
    static constexpr int FOOTER_Y = 296;
    static constexpr int START_Y = HEADER_H;
    static constexpr int ROWS_VISIBLE = 7;
    static constexpr int ROW_H = 36;
    static constexpr int CENTER_ROW = 3;
    static constexpr int SAFE_W = 390;
    static constexpr int ACCENT_W = 3;

    // Layout Columns
    static constexpr int COL_POS = 15;
    static constexpr int COL_NAME = 60;
    static constexpr int COL_PTS = SAFE_W - 15;

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _cursor;
    int _scrollOffset;

    void _renderHeader();
    void _renderFooter();
    void _renderRow(int row, int idx, int rowY);
    void _renderConnector();
    void _fullRedraw();
    void _updateScrollOffset();
    uint32_t _dimCol(uint32_t col, float b) const;
};