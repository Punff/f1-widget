#pragma once
#include "ScrollListView.h"

class ConstructorStandingsView : public ScrollListView
{
public:
    ConstructorStandingsView(LGFX *tft, DisplayManager *dm);

protected:
    // ScrollListView implementation
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onLongPress() override;
};
