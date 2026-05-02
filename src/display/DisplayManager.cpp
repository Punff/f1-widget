#include "DisplayManager.h"
#include "../../include/LGFX_Config.h"
#include <LittleFS.h>

void DisplayManager::drawSplash()
{
    _tft->fillScreen(UI::COL_BG);

    // LittleFS should already be mounted in main.cpp
    const char *path = "/F1-Logo-PNG-Cutout.png";

    if (LittleFS.exists(path))
    {
        File f = LittleFS.open(path, "r");
        if (f)
        {
            // drawPng returns true on success
            if (_tft->drawPng(&f, (size_t)f.size(), 0, 0))
            {
                f.close();
                return;
            }
            f.close();
        }
    }
    else
    {
        Serial.printf("[SPLASH] %s not found on LittleFS\n", path);
    }

    // Fallback: Draw F1 text logo
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(&fonts::Font8);
    _tft->drawString("F1", 240, 140);
    
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(&fonts::Font2);
    _tft->drawString("WIDGET", 240, 180);
    
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(&fonts::Font0);
    _tft->drawString("2026 Season", 240, 210);
}

DisplayManager::DisplayManager(LGFX *tft)
    : _tft(tft), _currentView(nullptr), _menuView(nullptr), _viewRegistry{}
{
}

void DisplayManager::init(IView *menuView)
{
    _menuView = menuView;
    _tft->fillScreen(UI::COL_BG);
    setView(menuView);
}

void DisplayManager::loop()
{
    if (_currentView)
        _currentView->tick();
}

void DisplayManager::setView(IView *view)
{
    if (_currentView)
        _currentView->onExit();

    _currentView = view;

    // Clean sweep instead of partial clears
    _tft->fillScreen(UI::COL_BG);

    _currentView->onEnter();
    _currentView->render();
}

void DisplayManager::returnToMenu()
{
    if (_weekendView) {
        delete _weekendView;
        _weekendView = nullptr;
    }
    setView(_menuView);
}

IView *DisplayManager::currentView() const { return _currentView; }

// ── Input Handlers (Forwarding directly to view) ─────────────────────────────

void DisplayManager::onTurnRight()
{
    if (_currentView)
        _currentView->onTurnRight();
}
void DisplayManager::onTurnLeft()
{
    if (_currentView)
        _currentView->onTurnLeft();
}
void DisplayManager::onPress()
{
    if (_currentView)
        _currentView->onPress();
}
void DisplayManager::onLongPress() { returnToMenu(); }
void DisplayManager::onDoublePress()
{
    if (_currentView)
        _currentView->onDoublePress();
}

void DisplayManager::registerView(MenuItem item, IView *view)
{
    int idx = static_cast<int>(item);
    if (idx >= 0 && idx < REGISTRY_SIZE)
        _viewRegistry[idx] = view;
}

void DisplayManager::launchMenuItem(int menuIndex)
{
    if (menuIndex >= 0 && menuIndex < REGISTRY_SIZE && _viewRegistry[menuIndex])
        setView(_viewRegistry[menuIndex]);
}

void DisplayManager::launchWeekendView(const RaceMeeting *meeting) {
    if (_weekendView) {
        delete _weekendView;
    }
    _weekendView = new WeekendView(_tft, this, meeting);
    setView(_weekendView);
}

LGFX *DisplayManager::tft() const { return _tft; }