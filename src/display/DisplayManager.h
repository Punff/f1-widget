#pragma once
#include "IView.h"
#include "views/UI.h"
#include "UI_Fonts.h"
#include "views/WeekendView.h"
#include "views/Header.h"
#include "views/Footer.h"

class LGFX;

class DisplayManager
{
public:
    explicit DisplayManager(LGFX *tft);

    void init(IView *menuView);
    void drawSplash();
    void drawBootStatus(const char *msg);
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
    LGFX_Sprite *rowSprite() const;
    Header *header() const;
    Footer *footer() const;

private:
    static constexpr int REGISTRY_SIZE = static_cast<int>(MenuItem::COUNT);

    LGFX *_tft;
    LGFX_Sprite *_sharedRowSprite;
    Header *_header;
    Footer *_footer;
    IView *_currentView;
    IView *_previousView;
    IView *_menuView;
    IView *_viewRegistry[REGISTRY_SIZE];
    WeekendView *_weekendView = nullptr;
};
