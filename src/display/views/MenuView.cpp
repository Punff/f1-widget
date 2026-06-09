#include "MenuView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"
#include "../../data/DataCache.h"
#include "../../time/TimeManager.h"
#include "../../time/TimeUtils.h"
#include <time.h>

extern DataCache *cache;
extern TimeManager *timeMgr;

// Column offsets (using shared UI constants)

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
    int ix = UI::COL_POS + 10;
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
    case MenuItem::CURRENT_WEEKEND:
        _rowSprite->fillTriangle(ix, iy - 8, ix - 6, iy, ix + 6, iy, iconCol);
        _rowSprite->fillTriangle(ix, iy + 8, ix - 6, iy, ix + 6, iy, iconCol);
        _rowSprite->fillCircle(ix, iy - 10, 3, iconCol);
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
    _rowSprite->drawString(_getMenuName(dataIdx), UI::COL_PRIMARY, _rowH / 2);
}

void MenuView::drawFooter()
{
    _dm->footer()->draw();
    char buf[48];
    snprintf(buf, sizeof(buf), "MEN \xc2\xb7 %s", _getMenuName(_cursor));
    _dm->footer()->drawCenter(buf, UI::COL_MUTED);
}

void MenuView::onPress()
{
    if (static_cast<MenuItem>(_cursor) == MenuItem::CURRENT_WEEKEND)
    {
        if (!timeMgr || !cache || cache->calendar.empty())
            return;

        time_t nowLocal = timeMgr->getLocalTime();
        struct tm nowTm;
        localtime_r(&nowLocal, &nowTm);
        char todayStr[16];
        strftime(todayStr, sizeof(todayStr), "%Y-%m-%d", &nowTm);

        const RaceMeeting *best = nullptr;

        // Find current weekend (today within [first session, race day])
        for (auto &rm : cache->calendar)
        {
            if (rm.sessionCount == 0) continue;
            char sd[16] = "";
            strncpy(sd, rm.sessions[0].dateUtc, 10);
            if (sd[0] && strcmp(todayStr, sd) >= 0 && strcmp(todayStr, rm.date) <= 0)
            {
                best = &rm;
                break;
            }
        }

        // Fallback: next upcoming weekend
        if (!best)
        {
            for (auto &rm : cache->calendar)
            {
                if (rm.sessionCount == 0) continue;
                char sd[16] = "";
                strncpy(sd, rm.sessions[0].dateUtc, 10);
                if (sd[0] && strcmp(sd, todayStr) > 0)
                {
                    best = &rm;
                    break;
                }
            }
        }

        // Last resort: last weekend of the season
        if (!best && !cache->calendar.empty())
            best = &cache->calendar.back();

        if (best)
            _dm->launchWeekendView(best);
        return;
    }

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
    case MenuItem::CURRENT_WEEKEND:       return "Race Weekend";
    case MenuItem::SETTINGS:              return "System Settings";
    default:                              return "Unknown";
    }
}
