#pragma once
#include "ScrollListView.h"
#include "../../data/DataCache.h"
#include <vector>

class WeekendView : public ScrollListView {
public:
    WeekendView(LGFX *tft, DisplayManager *dm, const RaceMeeting *meeting);
protected:
    int dataSize() const override;
    void onEnter() override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onPress() override;
    void onLongPress() override;
    void tick() override;
private:
    const RaceMeeting *_meeting;
    unsigned long _lastFooterSec = 0;
    std::vector<time_t> _sessionTimes;
};
