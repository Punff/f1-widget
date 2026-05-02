#pragma once
#include "ScrollListView.h"
#include "../../data/DataCache.h"

class WeekendView : public ScrollListView {
public:
    WeekendView(LGFX *tft, DisplayManager *dm, const RaceMeeting *meeting);
protected:
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onLongPress() override;
private:
    const RaceMeeting *_meeting;
};
