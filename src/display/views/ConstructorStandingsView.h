#pragma once
#ifndef CONSTRUCTOR_STANDINGS_VIEW_H
#define CONSTRUCTOR_STANDINGS_VIEW_H

#include "../IView.h"
#include "../../../include/LGFX_Config.h"
#include "../../data/DataCache.h"

class DisplayManager;

class ConstructorStandingsView : public IView
{
public:
    ConstructorStandingsView(LGFX *tft, DisplayManager *dm);

    void onEnter() override;
    void onExit() override;
    void render() override;
    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override;
    void onLongPress() override;
    void onDoublePress() override;

    static constexpr int ROW_HEIGHT = 40;
    static constexpr int ROWS_VISIBLE = 7;
    static constexpr int HEADER_H = 30;
    static constexpr int COL_POS_X = 8;
    static constexpr int COL_NAME_X = 50;
    static constexpr int COL_PTS_X = 400;

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _scrollOffset;

    void _renderHeader();
    void _renderRow(int rowY, int standing, bool highlight);
};

#endif