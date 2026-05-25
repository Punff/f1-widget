#pragma once
#include "ScrollListView.h"
#include "../../data/DataCache.h"
#include <vector>

class SessionResultsView : public ScrollListView
{
public:
    SessionResultsView(LGFX *tft, DisplayManager *dm, int round,
                       const char *officialName, const char *circuitName,
                       const char *sessionName);

protected:
    int dataSize() const override;
    void onEnter() override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onPress() override;
    void onLongPress() override;

private:
    int _meetingRound;
    char _officialName[96];
    char _circuitName[48];
    char _sessionName[32];
    std::vector<SessionResult> _results;
    bool _fetched;
    bool _fetchFailed;
};
