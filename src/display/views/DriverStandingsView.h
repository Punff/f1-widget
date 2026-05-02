#pragma once
#include "ScrollListView.h"

class DriverStandingsView : public ScrollListView
{
public:
    DriverStandingsView(LGFX *tft, DisplayManager *dm);

protected:
    // ScrollListView implementation
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;

private:
    // Column positions for 480px width
    static constexpr int COL_POS = 10;
    static constexpr int COL_TEAM_BAR = 40;
    static constexpr int COL_NAME = 50;
    static constexpr int COL_TEAM = 230;
    static constexpr int COL_PTS = 460;

    void drawPosition(int position, uint16_t teamColor, bool selected, int dist);
    void drawDriverName(const char *acronym, const char *lastName, bool selected, int dist);
    void drawTeamName(const char *teamName, bool selected, int dist);
};
