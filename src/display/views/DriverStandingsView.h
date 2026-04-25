#pragma once
#ifndef DRIVER_STANDINGS_VIEW_H
#define DRIVER_STANDINGS_VIEW_H

#include "../IView.h"
#include "../../../include/LGFX_Config.h"
#include "../../data/DataCache.h"

class DisplayManager;

class DriverStandingsView : public IView
{
public:
    DriverStandingsView(LGFX *tft, DisplayManager *dm);

    void onEnter() override;
    void onExit() override;
    void render() override;
    void onTurnRight() override;   // scroll down
    void onTurnLeft() override;    // scroll up
    void onPress() override;       // TODO: expand driver detail
    void onLongPress() override;   // back to menu — handled by DM
    void onDoublePress() override; // TODO: context TBD

    // Layout tweakables
    static constexpr int ROW_HEIGHT = 36;
    static constexpr int ROWS_VISIBLE = 8;
    static constexpr int HEADER_H = 30;
    static constexpr int COL_POS_X = 8;
    static constexpr int COL_CODE_X = 40;
    static constexpr int COL_NAME_X = 100;
    static constexpr int COL_PTS_X = 400;

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _scrollOffset; // first visible row index

    void _renderHeader();
    void _renderRow(int rowY, int standing, bool highlight);
};

#endif