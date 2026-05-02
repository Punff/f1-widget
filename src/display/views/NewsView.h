#pragma once
#include "../IView.h"
#include "UI.h"
#include "../../../include/LGFX_Config.h"

class DisplayManager;

class NewsView : public IView
{
public:
    NewsView(LGFX *tft, DisplayManager *dm);

    void onEnter() override;
    void onExit() override {}
    void render() override;
    void tick() override {}

    void onTurnRight() override {}
    void onTurnLeft() override {}
    void onPress() override {}
    void onLongPress() override {}
    void onDoublePress() override {}

private:
    LGFX *_tft;
    DisplayManager *_dm;

    void drawNewsHeader();
    void drawNewsContent();
};
