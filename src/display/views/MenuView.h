#pragma once
#include "../IView.h"
#include "UI.h"
#include "../../../include/LGFX_Config.h"

class LGFX;
class DisplayManager;

class MenuView : public IView
{
public:
    MenuView(LGFX *tft, DisplayManager *dm);

    // Lifecycle
    void onEnter() override;
    void onExit() override {}
    void render() override;
    void tick() override;

    // Input
    void onTurnRight() override;
    void onTurnLeft() override;
    void onPress() override;

private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _cursor;

    void drawMenu();
    void drawMenuItem(int index, bool selected);
    void partialRedraw(int oldCursor);
    const char *_getMenuName(int index);
    const char *_getMenuIcon(int index);
};
