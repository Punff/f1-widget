#include "MenuView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

MenuView::MenuView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _cursor(0) {}

void MenuView::onEnter()
{
    _cursor = 0;
    _tft->fillScreen(UI::COL_BG);
    drawMenu();
}

void MenuView::render()
{
    // No redraw needed - only redraw on input
}

void MenuView::tick()
{
    // Nothing to do
}

void MenuView::onTurnRight()
{
    if (_cursor < (int)MenuItem::COUNT - 1)
    {
        int oldCursor = _cursor;
        _cursor++;
        partialRedraw(oldCursor);
    }
}

void MenuView::onTurnLeft()
{
    if (_cursor > 0)
    {
        int oldCursor = _cursor;
        _cursor--;
        partialRedraw(oldCursor);
    }
}

void MenuView::onPress()
{
    _dm->launchMenuItem(_cursor);
}

void MenuView::drawMenu()
{
    _tft->startWrite();

    // Header - use centralized fonts
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // F1 Logo - use HEADER_BIG
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    // WIDGET - use BODY_MAIN
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("WIDGET", 75, 12);

    // Red separator line
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

    // Menu items
    int startY = UI::HEADER_H + 20;
    int itemHeight = 45;

    for (int i = 0; i < (int)MenuItem::COUNT; i++)
    {
        drawMenuItem(i, i == _cursor);
    }

    // Footer
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setTextDatum(bottom_right);
    _tft->drawString("v1.0.26", UI::SCREEN_W - 10, UI::SCREEN_H - 5);

    _tft->endWrite();
}

void MenuView::drawMenuItem(int index, bool selected)
{
    int startY = UI::HEADER_H + 20;
    int itemHeight = 45;
    int yPos = startY + (index * itemHeight);

    // Selection background
    if (selected)
    {
        _tft->fillRect(0, yPos, UI::SCREEN_W, itemHeight - 2, UI::COL_BG_SEL);
        // Red accent bar on left
        _tft->fillRect(0, yPos, 6, itemHeight - 2, UI::COL_F1_RED);
        // Right-side stripe for selected item
        _tft->fillRect(UI::SCREEN_W - 6, yPos, 6, itemHeight - 2, UI::COL_F1_RED);
    }
    else
    {
        _tft->fillRect(0, yPos, UI::SCREEN_W, itemHeight - 2, UI::COL_BG);
    }

    // Menu icon (first 2 chars)
    _tft->setTextDatum(middle_left);
    if (selected)
    {
        _tft->setTextColor(UI::COL_F1_RED);
        _tft->setFont(UI::Fonts::DATA_ACCENT);
    }
    else
    {
        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
    }
    _tft->drawString(_getMenuIcon(index), 14, yPos + (itemHeight / 2));

    // Menu name
    if (selected)
    {
        _tft->setTextColor(UI::COL_TEXT);
        _tft->setFont(UI::Fonts::BODY_MAIN);
    }
    else
    {
        _tft->setTextColor(UI::COL_TEXT_DIM);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
    }
    _tft->drawString(_getMenuName(index), 75, yPos + (itemHeight / 2));
}

void MenuView::partialRedraw(int oldCursor)
{
    _tft->startWrite();

    // Redraw old selected item (now unselected)
    if (oldCursor >= 0 && oldCursor < (int)MenuItem::COUNT)
    {
        drawMenuItem(oldCursor, false);
    }

    // Redraw new selected item
    drawMenuItem(_cursor, true);

    _tft->endWrite();
}

const char *MenuView::_getMenuName(int index)
{
    switch (static_cast<MenuItem>(index))
    {
    case MenuItem::DRIVER_STANDINGS:
        return "Driver Standings";
    case MenuItem::CONSTRUCTOR_STANDINGS:
        return "Constructor Standings";
    case MenuItem::CALENDAR:
        return "Season Calendar";
    case MenuItem::NEWS:
        return "Latest News";
    case MenuItem::SETTINGS:
        return "System Settings";
    default:
        return "Unknown";
    }
}

const char *MenuView::_getMenuIcon(int index)
{
    switch (static_cast<MenuItem>(index))
    {
    case MenuItem::DRIVER_STANDINGS:
        return "DS";
    case MenuItem::CONSTRUCTOR_STANDINGS:
        return "CS";
    case MenuItem::CALENDAR:
        return "CAL";
    case MenuItem::NEWS:
        return "NEW";
    case MenuItem::SETTINGS:
        return "SET";
    default:
        return "??";
    }
}
