#pragma once
#include "IView.h"
#include "views/UI.h"
#include "UI_Fonts.h"
#include "views/WeekendView.h"
#include "views/SessionResultsView.h"
#include "views/DriverDetailView.h"
#include "views/Header.h"
#include "views/Footer.h"

class LGFX;
class WifiSettingsView;
class DataSettingsView;

class DisplayManager
{
public:
    explicit DisplayManager(LGFX *tft);

    void init(IView *menuView);
    void drawSplash();
    void drawBootStatus(const char *msg);
    void drawWiFiInstructions();
    void loop();

    void setView(IView *view);
    void returnToMenu();
    void returnToPrevious();
    void returnToCalendar();
    IView *currentView() const;

    // ── Input callbacks ───────────────────────────────────────────────────────
    void onTurnRight();
    void onTurnLeft();
    void onPress();
    void onLongPress();
    void onDoublePress();

    void registerView(MenuItem item, IView *view);
    void launchMenuItem(int menuIndex);
    void setUserBrightness(uint8_t brightness);
    void setDisplayTimeout(uint16_t sec);

    void updateAutoSwitch();
    void launchWeekendView(const RaceMeeting *meeting);
    void launchSessionResultsView(const RaceMeeting *meeting, int sessionIdx);
    void launchDriverDetailView(int driverIdx);
    void launchWifiSettings();
    void launchDataSettings();

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
    WeekendView *_weekendView;
    SessionResultsView *_sessionResultsView;
    DriverDetailView *_driverDetailView;
    WifiSettingsView *_wifiSettingsView;
    DataSettingsView *_dataSettingsView;
    int _lastAutoSwitchRound;
    int _lastAutoSwitchSession;
    unsigned long _lastAutoSwitchMs;
    int _lastAutoSyncRound;
    uint16_t _displayTimeoutSec;
    unsigned long _lastActivityMs;
    bool _displayOff;
    uint8_t _userBrightness;
};
