#include "MenuView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

static constexpr int COL_ICON = 15;
static constexpr int COL_NAME = 70;

MenuView::MenuView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2) {}

int MenuView::dataSize() const
{
    return (int)MenuItem::COUNT;
}

void MenuView::drawHeader()
{
    _dm->header()->draw("WIDGET");
}

void MenuView::drawRow(int dataIdx, bool selected, int dist)
{
    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_F1_RED);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_F1_RED);
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    // Icon
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(selected ? UI::Fonts::DATA_ACCENT : UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(selected ? UI::COL_F1_RED : dim);
    _rowSprite->drawString(_getMenuIcon(dataIdx), COL_ICON, _rowH / 2);

    // Name
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(_getMenuName(dataIdx), COL_NAME, _rowH / 2);
}

void MenuView::drawFooter()
{
    _dm->footer()->draw();
    char buf[32];
    snprintf(buf, sizeof(buf), "Heap: %uK", ESP.getFreeHeap() / 1024);
    _dm->footer()->drawCenter(buf, UI::COL_MUTED);
}

void MenuView::onPress()
{
    _dm->launchMenuItem(_cursor);
}

void MenuView::onLongPress()
{
    _dm->returnToMenu();
}

const char *MenuView::_getMenuName(int index) const
{
    switch (static_cast<MenuItem>(index))
    {
    case MenuItem::DRIVER_STANDINGS:      return "Driver Standings";
    case MenuItem::CONSTRUCTOR_STANDINGS: return "Constructor Standings";
    case MenuItem::CALENDAR:              return "Season Calendar";
    case MenuItem::NEWS:                  return "Latest News";
    case MenuItem::SETTINGS:              return "System Settings";
    default:                              return "Unknown";
    }
}

const char *MenuView::_getMenuIcon(int index) const
{
    switch (static_cast<MenuItem>(index))
    {
    case MenuItem::DRIVER_STANDINGS:      return "DS";
    case MenuItem::CONSTRUCTOR_STANDINGS: return "CS";
    case MenuItem::CALENDAR:              return "CAL";
    case MenuItem::NEWS:                  return "NEW";
    case MenuItem::SETTINGS:              return "SET";
    default:                              return "??";
    }
}
