#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "IView.h"
#include "EncoderWidget.h"
#include "../config.h"

class LGFX;

class DisplayManager
{
public:
    DisplayManager(LGFX *tft);

    void init(IView *menuView);
    void loop();

    void setView(IView *view);
    void returnToMenu();
    IView *currentView() const;

    // Encoder input — call from main.cpp callbacks
    void onTurnRight();
    void onTurnLeft();
    void onPress();
    void onLongPress();
    void onDoublePress();

    // Menu item registry
    void registerView(int menuIndex, IView *view);
    void launchMenuItem(int menuIndex);

    // Helpers available to views
    void clearContent();
    LGFX *tft() const;

    static constexpr int CONTENT_W = 440;
    static constexpr int CONTENT_H = 320;

private:
    LGFX *_tft;
    IView *_currentView;
    IView *_menuView;
    EncoderWidget _widget;
    IView *_viewRegistry[8];
};

#endif