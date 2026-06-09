#pragma once
#include "ScrollListView.h"

class DataSettingsView : public ScrollListView
{
public:
    DataSettingsView(LGFX *tft, DisplayManager *dm);

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
    enum DataRow {
        ROW_SYNC_ALL = 0,
        ROW_SYSINFO,
        ROW_CLEAR_CACHE,
        ROW_COUNT
    };

    bool _syncing;
    unsigned long _syncStart;
    time_t _lastSyncTime;
};
