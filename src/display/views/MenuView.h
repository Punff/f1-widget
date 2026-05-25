#pragma once
#include "ScrollListView.h"
#include "UI.h"

class MenuView : public ScrollListView
{
public:
    MenuView(LGFX *tft, DisplayManager *dm);

protected:
    int dataSize() const override;
    void drawHeader() override;
    void drawRow(int dataIdx, bool selected, int dist) override;
    void drawFooter() override;
    void onPress() override;

private:
    const char *_getMenuName(int index) const;
    const char *_getMenuIcon(int index) const;
};
