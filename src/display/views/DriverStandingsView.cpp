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
    : ScrollListView(tft, dm, 48, 5, 2) // RowH=48px, 5 rows, selection at index 2
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
    _tft->drawString("#", COL_POS, 34);
    _tft->drawString("DRIVER", COL_NAME, 34);
    _tft->drawString("TEAM", COL_TEAM, 34);
    _tft->setTextDatum(top_right);
    _tft->drawString("PTS", COL_PTS, 34);
}

void DriverStandingsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= (int)cache->driverStandings.size()) return;
    const auto &ds = cache->driverStandings[dataIdx];
    uint16_t tc = ds.driver.team.teamColor;

    uint32_t textCol = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);
    uint32_t nameCol = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    // Team color bar on left (always full brightness)
    _rowSprite->fillRect(0, 0, 4, _rowH, tc);

    if (selected) {
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
    _rowSprite->drawNumber(ds.points, COL_PTS, _rowH / 2);
}

void DriverStandingsView::drawFooter()
{
    _dm->footer()->draw();

    if (cache->driverStandings.empty() || _cursor >= (int)cache->driverStandings.size())
        return;

    const auto &sel = cache->driverStandings[_cursor];
    const auto &leader = cache->driverStandings[0];
    int gap = leader.points - sel.points;

    char buf[64];
    uint32_t color;
    if (_cursor == 0)
    {
        color = sel.driver.team.teamColor;
        snprintf(buf, sizeof(buf), "LEADER: %s", sel.driver.fullName);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        snprintf(buf, sizeof(buf), "GAP TO P1: +%d PTS", gap);
    }

    _dm->footer()->drawText(buf, color);
}

void DriverStandingsView::onLongPress()
{
    _dm->returnToMenu();
}
