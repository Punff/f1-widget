#include "DisplayManager.h"
#include "../../include/LGFX_Config.h"
#include "../time/TimeManager.h"
#include <WiFi.h>
#include <LittleFS.h>

extern TimeManager *timeMgr;

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
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", UI::SCREEN_W / 2, 140);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("WIDGET", UI::SCREEN_W / 2, 170);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("2026 Season", UI::SCREEN_W / 2, 200);
}

void DisplayManager::drawBootStatus(const char *msg)
{
    _tft->fillRect(0, 260, UI::SCREEN_W, 50, UI::COL_BG);
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(msg, UI::SCREEN_W / 2, 285);

    // Also output to serial for debugging
    Serial.printf("[BOOT] %s\n", msg);
}

DisplayManager::DisplayManager(LGFX *tft)
    : _tft(tft), _header(nullptr), _footer(nullptr),
      _currentView(nullptr), _previousView(nullptr),
      _menuView(nullptr), _viewRegistry{}
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
            _footer->setWifiConnected(wifiOk);
            _footer->redrawWifi();
        }
    }
}

void DisplayManager::setView(IView *view)
{
    if (_currentView)
        _currentView->onExit();

    // Keep WeekendView alive when entering SessionResultsView
    if (_currentView == _weekendView && view != _sessionResultsView) {
        delete _weekendView;
        _weekendView = nullptr;
    }
    if (_currentView == _sessionResultsView) {
        delete _sessionResultsView;
        _sessionResultsView = nullptr;
    }

    _currentView = view;

    _header->markDirty();

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
    if (_header) _header->encoderPulse(1);
    if (_currentView)
        _currentView->onTurnRight();
}
void DisplayManager::onTurnLeft()
{
    if (_header) _header->encoderPulse(-1);
    if (_currentView)
        _currentView->onTurnLeft();
}
void DisplayManager::onPress()
{
    if (_header) _header->encoderPress();
    if (_currentView)
        _currentView->onPress();
}
void DisplayManager::onLongPress()
{
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

void DisplayManager::launchWeekendView(const RaceMeeting *meeting) {
    if (_weekendView) {
        delete _weekendView;
    }
    _previousView = _currentView;
    _sessionResultsView = nullptr;
    _weekendView = new WeekendView(_tft, this, meeting);
    setView(_weekendView);
}

void DisplayManager::launchSessionResultsView(const RaceMeeting *meeting, int sessionIdx) {
    if (_sessionResultsView) {
        delete _sessionResultsView;
    }
    _previousView = _currentView;
    _sessionResultsView = new SessionResultsView(_tft, this,
        meeting->round, meeting->officialName, meeting->circuit.shortName,
        meeting->sessions[sessionIdx].name);
    setView(_sessionResultsView);
}

LGFX *DisplayManager::tft() const { return _tft; }
LGFX_Sprite *DisplayManager::rowSprite() const { return _sharedRowSprite; }
Header *DisplayManager::header() const { return _header; }
Footer *DisplayManager::footer() const { return _footer; }
