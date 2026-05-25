#pragma once
#include "ScrollListView.h"

class DriverStandingsView : public ScrollListView
{
public:
    DriverStandingsView(LGFX *tft, DisplayManager *dm);

protected:
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onTurnRight() override;
    void onTurnLeft() override;
    void onLongPress() override;
};
