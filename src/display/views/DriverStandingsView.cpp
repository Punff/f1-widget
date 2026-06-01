#include "DriverStandingsView.h"
#include "../DisplayManager.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

// Column offsets
static constexpr int COL_POS = 15;
static constexpr int COL_NAME = 55;
static constexpr int COL_TEAM = 160;
static constexpr int COL_PTS = 465;

DriverStandingsView::DriverStandingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2) // RowH=46px, 5 rows, selection at index 2
{
}

int DriverStandingsView::dataSize() const
{
    return cache->driverStandings.size();
}

void DriverStandingsView::drawHeader()
{
    _dm->header()->draw("DRIVER STANDINGS");

    // Column headers
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);

    _tft->setTextDatum(middle_left);
    _tft->drawString("#", COL_POS, 42);
    _tft->drawString("DRIVER", COL_NAME, 42);
    _tft->drawString("TEAM", COL_TEAM, 42);

    _tft->setTextDatum(middle_right);
    _tft->drawString("PTS", COL_PTS, 42);
}

void DriverStandingsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= (int)cache->driverStandings.size())
        return;
    const auto &ds = cache->driverStandings[dataIdx];
    uint16_t tc = ds.driver.team.teamColor;

    uint32_t textCol = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);
    uint32_t nameCol = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    // Team color bar on left (always full brightness)
    _rowSprite->fillRect(0, 0, 4, _rowH, tc);

    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, tc);
    }

    // Position — team color
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(tc);
    char posBuf[4];
    snprintf(posBuf, sizeof(posBuf), "%2d", ds.position);
    _rowSprite->drawString(posBuf, COL_POS, _rowH / 2);

    // Driver acronym
    _rowSprite->setTextColor(nameCol);
    _rowSprite->drawString(ds.driver.acronym, COL_NAME, _rowH / 2);

    // Team name
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(textCol);
    _rowSprite->drawString(ds.driver.team.name, COL_TEAM, _rowH / 2);

    // Points
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(tc);
    char ptsStr[16];
    if (ds.points == (int)ds.points) snprintf(ptsStr, sizeof(ptsStr), "%d", (int)ds.points);
    else snprintf(ptsStr, sizeof(ptsStr), "%.1f", ds.points);
    _rowSprite->drawString(ptsStr, COL_PTS, _rowH / 2);
}

void DriverStandingsView::drawFooter()
{
    _dm->footer()->draw();

    if (cache->driverStandings.empty() || _cursor >= (int)cache->driverStandings.size())
        return;

    const auto &sel = cache->driverStandings[_cursor];
    const auto &leader = cache->driverStandings[0];
    float gap = leader.points - sel.points;

    char buf[48];
    uint32_t color;
    if (_cursor == 0)
    {
        uint16_t tc = sel.driver.team.teamColor;
        color = ((tc & 0xF800) << 8) | ((tc & 0x07E0) << 5) | ((tc & 0x001F) << 3);
        snprintf(buf, sizeof(buf), "DS \xc2\xb7 P1: %s", sel.driver.acronym);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        if (gap == (int)gap) snprintf(buf, sizeof(buf), "DS \xc2\xb7 Gap: +%d", (int)gap);
        else snprintf(buf, sizeof(buf), "DS \xc2\xb7 Gap: +%.1f", gap);
    }

    _dm->footer()->drawCenter(buf, color);
}

void DriverStandingsView::onTurnRight()
{
    ScrollListView::onTurnRight();
}

void DriverStandingsView::onTurnLeft()
{
    ScrollListView::onTurnLeft();
}

void DriverStandingsView::onLongPress()
{
    _dm->returnToMenu();
}
