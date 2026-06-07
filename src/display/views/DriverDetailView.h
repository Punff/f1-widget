#pragma once
#include "../IView.h"
#include "UI.h"

class LGFX;
class DisplayManager;

class DriverDetailView : public IView
{
public:
    DriverDetailView(LGFX *tft, DisplayManager *dm, int driverIdx);
    void onEnter() override;
    void onExit() override {}
    void render() override;
    void tick() override {}
    void onLongPress() override;
    void onPress() override {}
    void onTurnLeft() override;
    void onTurnRight() override;
private:
    LGFX *_tft;
    DisplayManager *_dm;
    int _driverIdx;
    void drawCard();
};
