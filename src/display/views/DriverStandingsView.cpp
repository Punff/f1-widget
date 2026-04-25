#include "DriverStandingsView.h"
#include "../DisplayManager.h"
#include "../../../include/LGFX_Config.h"

DriverStandingsView::DriverStandingsView(LGFX* tft, DisplayManager* dm)
    : _tft(tft), _dm(dm), _scrollOffset(0) {}

void DriverStandingsView::onEnter()       {}
void DriverStandingsView::onExit()        {}
void DriverStandingsView::render()        {}
void DriverStandingsView::onTurnRight()   {}
void DriverStandingsView::onTurnLeft()    {}
void DriverStandingsView::onPress()       {}
void DriverStandingsView::onLongPress()   {}
void DriverStandingsView::onDoublePress() {}