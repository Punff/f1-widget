#include "DriverStandingsView.h"
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
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    _tft->setFont(UI::Fonts::HEADER_MEDIUM);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->drawString("DRIVER STANDINGS", 75, 10);

    // Red separator
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

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

    float brightness = rowBrightness(dist);
    uint32_t textCol = selected ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);
    uint32_t dimTextCol = selected ? UI::COL_TEXT_DIM : dimCol(UI::COL_TEXT_DIM, brightness);
    uint32_t teamCol = ds.driver.team.teamColor;
    if (!selected) teamCol = dimCol(teamCol, brightness);

    // Team color bar
    _rowSprite->fillRect(0, 0, 4, _rowH, ds.driver.team.teamColor);

    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, ds.driver.team.teamColor);
    }

    // Position
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(selected ? UI::Fonts::DATA_ACCENT : UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(selected ? teamCol : textCol);
    _rowSprite->drawNumber(ds.position, COL_POS, _rowH / 2);

    // Driver Name: Full name if selected, acronym if not
    _rowSprite->setTextColor(UI::COL_TEXT);
    if (selected) {
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->drawString(ds.driver.lastName, COL_NAME, _rowH / 2);
    } else {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
        _rowSprite->drawString(ds.driver.acronym, COL_NAME, _rowH / 2);
    }

    // Team (Subtle)
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(dimTextCol);
    _rowSprite->drawString(ds.driver.team.name, COL_TEAM, _rowH / 2);

    // Points
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(selected ? UI::Fonts::DATA_ACCENT : UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(selected ? teamCol : textCol);
    _rowSprite->drawNumber(ds.points, COL_PTS, _rowH / 2);
}

void DriverStandingsView::drawFooter()
{
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

    drawFooterText(buf, UI::SCREEN_W - 10, UI::FOOTER_Y + (UI::FOOTER_H / 2), color, UI::FONT_SMALL);
}
