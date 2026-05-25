#pragma once
#include "ScrollListView.h"
#include <time.h>

class CalendarView : public ScrollListView
{
public:
    CalendarView(LGFX *tft, DisplayManager *dm);

    int dataSize() const override;
    void onEnter() override;
    void onPress() override;
    void onLongPress() override;
    void tick() override;

    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;

private:
    time_t _lastFooterSec;
};
