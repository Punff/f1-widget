#pragma once
#include "IView.h"
#include "views/UI.h"
#include "UI_Fonts.h"
#include "views/WeekendView.h"

class LGFX;

class DisplayManager
{
public:
    explicit DisplayManager(LGFX *tft);

    void init(IView *menuView);
    void drawSplash();
    void loop();

    void setView(IView *view);
    void returnToMenu();
    IView *currentView() const;

    // ── Input callbacks ───────────────────────────────────────────────────────
    void onTurnRight();
    void onTurnLeft();
    void onPress();
    void onLongPress();
    void onDoublePress();

    void registerView(MenuItem item, IView *view);
    void launchMenuItem(int menuIndex);
    void launchWeekendView(const RaceMeeting *meeting);

    LGFX *tft() const;

private:
    static constexpr int REGISTRY_SIZE = static_cast<int>(MenuItem::COUNT);

    LGFX *_tft;
    IView *_currentView;
    IView *_menuView;
    IView *_viewRegistry[REGISTRY_SIZE];
    WeekendView *_weekendView = nullptr;
};