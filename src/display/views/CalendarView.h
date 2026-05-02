#pragma once
#include "ScrollListView.h"

class CalendarView : public ScrollListView
{
public:
    CalendarView(LGFX *tft, DisplayManager *dm);

protected:
    // ScrollListView implementation
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
};
