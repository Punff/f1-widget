#include "DisplayManager.h"
#include "../../include/LGFX_Config.h"

DisplayManager::DisplayManager(LGFX *tft)
    : _tft(tft), _currentView(nullptr), _menuView(nullptr), _widget(tft), _viewRegistry{}
{
}

void DisplayManager::init(IView *menuView)
{
    _menuView = menuView;
    _tft->fillScreen(0x000000);
    _widget.init();
    setView(menuView);
}

void DisplayManager::loop()
{
    _widget.loop();
    if (_currentView)
        _currentView->tick();
}

void DisplayManager::setView(IView *view)
{
    if (_currentView)
        _currentView->onExit();
    _currentView = view;
    clearContent();
    _currentView->onEnter();
    _currentView->render();
}

void DisplayManager::returnToMenu()
{
    setView(_menuView);
}

IView *DisplayManager::currentView() const
{
    return _currentView;
}

void DisplayManager::onTurnRight()
{
    _widget.notify(EncoderWidgetState::TURN_UP);
    if (_currentView)
        _currentView->onTurnRight();
}

void DisplayManager::onTurnLeft()
{
    _widget.notify(EncoderWidgetState::TURN_DOWN);
    if (_currentView)
        _currentView->onTurnLeft();
}

void DisplayManager::onPress()
{
    _widget.notify(EncoderWidgetState::PRESS);
    if (_currentView)
        _currentView->onPress();
}

void DisplayManager::onLongPress()
{
    _widget.notify(EncoderWidgetState::LONG_PRESS);
    returnToMenu();
}

void DisplayManager::onDoublePress()
{
    _widget.notify(EncoderWidgetState::DOUBLE_PRESS);
    if (_currentView)
        _currentView->onDoublePress();
}

void DisplayManager::registerView(int menuIndex, IView *view)
{
    if (menuIndex >= 0 && menuIndex < 8)
    {
        _viewRegistry[menuIndex] = view;
    }
}

void DisplayManager::launchMenuItem(int menuIndex)
{
    if (menuIndex >= 0 && menuIndex < 8 && _viewRegistry[menuIndex])
    {
        setView(_viewRegistry[menuIndex]);
    }
}

void DisplayManager::clearContent()
{
    _tft->fillRect(0, 0, 390, CONTENT_H, 0x000000);
}

LGFX *DisplayManager::tft() const
{
    return _tft;
}