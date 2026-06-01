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
        _rowSprite->fillRect(0, 0, 4, _rowH, UI::COL_ACCENT);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, UI::COL_ACCENT);
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);
    uint32_t iconCol = selected ? UI::COL_ACCENT : dim;

    // Geometric Icon
    int ix = COL_ICON + 10;
    int iy = _rowH / 2;
    switch (static_cast<MenuItem>(dataIdx))
    {
    case MenuItem::DRIVER_STANDINGS:
        _rowSprite->fillCircle(ix, iy, 8, iconCol);
        _rowSprite->fillCircle(ix, iy, 4, UI::COL_BG);
        break;
    case MenuItem::CONSTRUCTOR_STANDINGS:
        _rowSprite->fillRect(ix - 8, iy - 6, 16, 12, iconCol);
        break;
    case MenuItem::CALENDAR:
        _rowSprite->drawRect(ix - 8, iy - 8, 16, 16, iconCol);
        _rowSprite->drawFastHLine(ix - 8, iy - 2, 16, iconCol);
        break;
    case MenuItem::NEWS:
        _rowSprite->drawFastVLine(ix - 8, iy - 8, 16, iconCol);
        _rowSprite->drawFastHLine(ix - 8, iy - 8, 12, iconCol);
        _rowSprite->drawFastHLine(ix - 8, iy, 8, iconCol);
        _rowSprite->drawFastHLine(ix - 8, iy + 8, 14, iconCol);
        break;
    case MenuItem::SETTINGS:
        _rowSprite->drawCircle(ix, iy, 7, iconCol);
        for(int a=0; a<360; a+=45) {
            float rad = a * DEG_TO_RAD;
            _rowSprite->fillCircle(ix + cos(rad)*9, iy + sin(rad)*9, 2, iconCol);
        }
        break;
    default:
        break;
    }

    // Name
    _rowSprite->setTextDatum(middle_left);
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
