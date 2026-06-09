#include "DisplayManager.h"
#include "views/WifiSettingsView.h"
#include "views/DataSettingsView.h"
#include "../../include/LGFX_Config.h"
#include "../time/TimeManager.h"
#include "../time/TimeUtils.h"
#include "../data/DataCache.h"
#include "../api/APIClient.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <time.h>

uint32_t UI::COL_ACCENT = UI::COL_ACCENT_DEFAULT;

extern TimeManager *timeMgr;
extern DataCache *cache;
extern APIClient *api;

void DisplayManager::drawSplash()
{
    _tft->fillScreen(UI::COL_BG);

    // Load PNG into memory, then draw centered (LGFX has no fs::File DataWrapper)
    File f = LittleFS.open("/F1-Logo-PNG-Cutout.png", "r");
    if (f)
    {
        size_t sz = (size_t)f.size();
        uint8_t *buf = (uint8_t *)malloc(sz);
        if (buf)
        {
            if ((size_t)f.read(buf, sz) == sz)
            {
                _tft->drawPng(buf, sz, 144, 64);
            }
            free(buf);
            f.close();
            return;
        }
        f.close();
    }

    // Text fallback centered on screen
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_ACCENT);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", UI::SCREEN_W / 2, 140);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("WIDGET", UI::SCREEN_W / 2, 170);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    char seasonStr[16];
    snprintf(seasonStr, sizeof(seasonStr), "%d Season", cache ? cache->currentSeason : 2026);
    _tft->drawString(seasonStr, UI::SCREEN_W / 2, 200);
}

void DisplayManager::drawBootStatus(const char *msg)
{
    _tft->fillRect(0, 260, UI::SCREEN_W, 60, UI::COL_BG);
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(msg, UI::SCREEN_W / 2, 285);

    Serial.printf("[BOOT] %s\n", msg);
}

void DisplayManager::drawWiFiInstructions()
{
    int cx = UI::SCREEN_W / 2;
    int y = 210;

    _tft->fillRect(0, 195, UI::SCREEN_W, 65, UI::COL_BG);
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_ACCENT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("WiFi Setup Required", cx, y);

    _tft->setTextColor(UI::COL_TEXT_DIM);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("Connect to  \"F1Widget\"  WiFi network", cx, y + 20);
    _tft->drawString("then open a browser to 192.168.4.1", cx, y + 36);
}

DisplayManager::DisplayManager(LGFX *tft)
    : _tft(tft), _header(nullptr), _footer(nullptr),
      _currentView(nullptr), _previousView(nullptr),
      _menuView(nullptr), _viewRegistry{},
      _weekendView(nullptr), _sessionResultsView(nullptr),
      _driverDetailView(nullptr), _wifiSettingsView(nullptr), _dataSettingsView(nullptr),
      _lastAutoSwitchRound(-1), _lastAutoSwitchSession(-1), _lastAutoSwitchMs(0), _lastAutoSyncRound(-1),
      _displayTimeoutSec(0), _lastActivityMs(0), _displayOff(false), _userBrightness(255)
{
    _sharedRowSprite = new LGFX_Sprite(_tft);
    // Fixed size — never reallocated. All views use same MAX_ROW_H.
    // This prevents heap fragmentation from views with different row heights.
    if (!_sharedRowSprite->createSprite(UI::SCREEN_W, UI::MAX_ROW_H)) {
        Serial.println("[ERROR] Shared row sprite allocation failed");
    }
    _header = new Header(_tft);
    _footer = new Footer(_tft);
}

void DisplayManager::init(IView *menuView)
{
    _menuView = menuView;
    _tft->fillScreen(UI::COL_BG);
    setView(menuView);
}

void DisplayManager::loop()
{
    _header->tick(timeMgr);

    if (_currentView)
        _currentView->tick();

    static unsigned long lastWifiCheck = 0;
    static bool lastWifiOk = false;
    unsigned long now = millis();
    if (now - lastWifiCheck >= 1000) {
        lastWifiCheck = now;
        bool wifiOk = (WiFi.status() == WL_CONNECTED);
        if (wifiOk != lastWifiOk) {
            lastWifiOk = wifiOk;
            if (wifiOk && timeMgr && !timeMgr->isSynced())
                timeMgr->syncNTP();
            _footer->setWifiConnected(wifiOk);
            _footer->redrawWifi();
        }

        // Display timeout — dim backlight when idle
        if (_displayTimeoutSec > 0 && !_displayOff &&
            now - _lastActivityMs > (unsigned long)_displayTimeoutSec * 1000)
        {
            _displayOff = true;
            ledcWrite(0, 0);
        }
    }

    // Auto-switch to current weekend when a session goes live
    if (now - _lastAutoSwitchMs >= 60000) {
        _lastAutoSwitchMs = now;
        updateAutoSwitch();
    }
}

void DisplayManager::setUserBrightness(uint8_t brightness)
{
    _userBrightness = brightness;
}

void DisplayManager::setDisplayTimeout(uint16_t sec)
{
    _displayTimeoutSec = sec;
}

void DisplayManager::setView(IView *view)
{
    if (_currentView)
        _currentView->onExit();

    // Keep WeekendView alive when entering SessionResultsView
    if (_currentView == _weekendView && view != _sessionResultsView) {
        if (_previousView == _weekendView) _previousView = nullptr;
        delete _weekendView;
        _weekendView = nullptr;
    }
    if (_currentView == _sessionResultsView) {
        if (_previousView == _sessionResultsView) _previousView = nullptr;
        delete _sessionResultsView;
        _sessionResultsView = nullptr;
    }
    if (_currentView == _driverDetailView) {
        if (_previousView == _driverDetailView) _previousView = nullptr;
        delete _driverDetailView;
        _driverDetailView = nullptr;
    }
    if (_currentView == _wifiSettingsView) {
        if (_previousView == _wifiSettingsView) _previousView = nullptr;
        delete _wifiSettingsView;
        _wifiSettingsView = nullptr;
    }
    if (_currentView == _dataSettingsView) {
        if (_previousView == _dataSettingsView) _previousView = nullptr;
        delete _dataSettingsView;
        _dataSettingsView = nullptr;
    }

    _currentView = view;
    _lastActivityMs = millis();

    _header->markDirty();
    _footer->markDirty();

    // Clean sweep instead of partial clears
    _tft->fillScreen(UI::COL_BG);

    _currentView->onEnter();
    _currentView->render();
}

void DisplayManager::returnToMenu()
{
    setView(_menuView);
}

void DisplayManager::returnToPrevious()
{
    if (_previousView && _previousView != _currentView && _previousView != _menuView) {
        IView *target = _previousView;
        _previousView = nullptr;
        setView(target);
        return;
    }
    returnToMenu();
}

void DisplayManager::returnToCalendar()
{
    int idx = static_cast<int>(MenuItem::CALENDAR);
    if (idx >= 0 && idx < REGISTRY_SIZE && _viewRegistry[idx]) {
        _previousView = _currentView;
        setView(_viewRegistry[idx]);
        return;
    }
    returnToMenu();
}

IView *DisplayManager::currentView() const { return _currentView; }

// ── Input Handlers ─────────────────────────────────────────────────────────

void DisplayManager::onTurnRight()
{
    _lastActivityMs = millis();
    if (_displayOff) { _displayOff = false; ledcWrite(0, _userBrightness); }
    if (_header) _header->encoderPulse(1);
    if (_currentView)
        _currentView->onTurnRight();
}
void DisplayManager::onTurnLeft()
{
    _lastActivityMs = millis();
    if (_displayOff) { _displayOff = false; ledcWrite(0, _userBrightness); }
    if (_header) _header->encoderPulse(-1);
    if (_currentView)
        _currentView->onTurnLeft();
}
void DisplayManager::onPress()
{
    _lastActivityMs = millis();
    if (_displayOff) { _displayOff = false; ledcWrite(0, _userBrightness); }
    if (_header) _header->encoderPress();
    if (_currentView)
        _currentView->onPress();
}
void DisplayManager::onLongPress()
{
    _lastActivityMs = millis();
    if (_displayOff) { _displayOff = false; ledcWrite(0, _userBrightness); }
    if (_header) _header->encoderLongPress();
    if (_currentView) _currentView->onLongPress();
}
void DisplayManager::onDoublePress()
{
    returnToPrevious();
}

void DisplayManager::registerView(MenuItem item, IView *view)
{
    int idx = static_cast<int>(item);
    if (idx >= 0 && idx < REGISTRY_SIZE)
        _viewRegistry[idx] = view;
}

void DisplayManager::launchMenuItem(int menuIndex)
{
    if (menuIndex >= 0 && menuIndex < REGISTRY_SIZE && _viewRegistry[menuIndex]) {
        _previousView = _currentView;
        setView(_viewRegistry[menuIndex]);
    }
}

void DisplayManager::updateAutoSwitch()
{
    if (!timeMgr || !timeMgr->isSynced() || !cache || cache->calendar.empty())
        return;

    time_t nowLocal = timeMgr->getLocalTime();
    struct tm nowTm;
    localtime_r(&nowLocal, &nowTm);
    char todayStr[16];
    strftime(todayStr, sizeof(todayStr), "%Y-%m-%d", &nowTm);

    // Find the weekend that is currently active
    for (auto &rm : cache->calendar)
    {
        if (rm.sessionCount == 0) continue;
        char sd[16] = "";
        strncpy(sd, rm.sessions[0].dateUtc, 10);
        if (sd[0] && strcmp(todayStr, sd) >= 0 && strcmp(todayStr, rm.date) <= 0)
        {
            if (rm.round != _lastAutoSwitchRound)
            {
                _lastAutoSwitchRound = rm.round;
                _lastAutoSwitchSession = 0;
                launchWeekendView(&rm);
            }
            return;
        }
    }

    // Auto-sync: if any weekend's race date has passed, sync leaderboard data
    for (auto &rm : cache->calendar)
    {
        if (rm.sessionCount == 0) continue;
        if (strcmp(rm.date, todayStr) < 0 && rm.round > _lastAutoSyncRound)
        {
            _lastAutoSyncRound = rm.round;
            if (api) api->syncAll();
            return;
        }
    }
}

void DisplayManager::launchWeekendView(const RaceMeeting *meeting) {
    if (_weekendView) {
        if (_currentView == _weekendView)
            _currentView = nullptr;
        delete _weekendView;
    }
    _previousView = _currentView;
    _sessionResultsView = nullptr;
    _weekendView = new WeekendView(_tft, this, meeting->round);
    setView(_weekendView);
}

void DisplayManager::launchSessionResultsView(const RaceMeeting *meeting, int sessionIdx) {
    if (_sessionResultsView) {
        if (_currentView == _sessionResultsView)
            _currentView = nullptr;
        delete _sessionResultsView;
    }
    _previousView = _currentView;
    _sessionResultsView = new SessionResultsView(_tft, this,
        meeting->round, meeting->officialName, meeting->circuit.shortName,
        meeting->sessions[sessionIdx].name);
    setView(_sessionResultsView);
}

void DisplayManager::launchDriverDetailView(int driverIdx) {
    if (_driverDetailView) {
        if (_currentView == _driverDetailView)
            _currentView = nullptr;
        delete _driverDetailView;
    }
    _previousView = _currentView;
    _driverDetailView = new DriverDetailView(_tft, this, driverIdx);
    setView(_driverDetailView);
}

void DisplayManager::launchWifiSettings() {
    if (_wifiSettingsView) {
        if (_currentView == _wifiSettingsView) _currentView = nullptr;
        delete _wifiSettingsView;
    }
    _previousView = _currentView;
    _wifiSettingsView = new WifiSettingsView(_tft, this);
    setView(_wifiSettingsView);
}

void DisplayManager::launchDataSettings() {
    if (_dataSettingsView) {
        if (_currentView == _dataSettingsView) _currentView = nullptr;
        delete _dataSettingsView;
    }
    _previousView = _currentView;
    _dataSettingsView = new DataSettingsView(_tft, this);
    setView(_dataSettingsView);
}

LGFX *DisplayManager::tft() const { return _tft; }
LGFX_Sprite *DisplayManager::rowSprite() const { return _sharedRowSprite; }
Header *DisplayManager::header() const { return _header; }
Footer *DisplayManager::footer() const { return _footer; }
