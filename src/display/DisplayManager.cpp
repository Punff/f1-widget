#include "DisplayManager.h"
#include "../../include/LGFX_Config.h"
#include <LittleFS.h>

void DisplayManager::drawSplash()
{
    _tft->fillScreen(UI::COL_BG);

    // Ensure we use LittleFS here
    if (!LittleFS.begin())
    {
        Serial.println("[ERROR] LittleFS Mount Failed!");
    }
    else
    {
        // Path must be root '/'
        const char *path = "/F1-Logo-PNG-Cutout.png";

        if (LittleFS.exists(path))
        {
            File f = LittleFS.open(path, "r");
            if (f)
            {
                bool success = _tft->drawPng(&f, (size_t)f.size(), 0, 0);
                f.close();
                if (success)
                    return;
            }
        }
        else
        {
            Serial.printf("[ERROR] %s not found on LittleFS\n", path);
        }
    }

    // Fallback drawing...
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

LGFX *DisplayManager::tft() const { return _tft; }