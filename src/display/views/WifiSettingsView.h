#pragma once
#include "ScrollListView.h"

class WifiSettingsView : public ScrollListView
{
public:
    WifiSettingsView(LGFX *tft, DisplayManager *dm);

protected:
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onEnter() override;
    void onPress() override;
    void onLongPress() override;
    void tick() override;

private:
    enum RowType {
        ROW_ADD_HOTSPOT = 0,  // offset from saved networks end
        ROW_FORGET = 1,
        ROW_COUNT_BASE = 2
    };

    int _numNetworks;
    bool _connecting;
    unsigned long _connectingStart;
};
