#include "DriverStandingsView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

DriverStandingsView::DriverStandingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 48, 6, 2) // RowH=48px, 6 rows, selection at index 2
{
}

int DriverStandingsView::dataSize() const
{
    return cache->driverStandings.size();
}

void DriverStandingsView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title - use centralized fonts
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
    _tft->drawString("PTS", COL_PTS - 25, 34);
}
// Typography - F1 Font Mappings
// Use setFont() instead of setTextSize()
// FONT_TINY  -> F1_FONT_REGULAR12
// FONT_SMALL -> F1_FONT_REGULAR12
// FONT_MEDIUM -> F1_FONT_REGULAR16
// FONT_LARGE -> F1_FONT_WIDE20
// F1 Logo/Headers -> F1_FONT_BOLD16 or F1_FONT_WIDE20
void DriverStandingsView::drawRow(int dataIdx, bool selected, int dist)
{
    const auto &ds = cache->driverStandings[dataIdx];

    // Calculate brightness
    float brightness = rowBrightness(dist);
    uint32_t textCol = selected ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);
    uint32_t ptsCol = selected ? ds.driver.team.teamColor : dimCol(UI::COL_TEXT, brightness);

    // Team color bar on left edge
    _rowSprite->fillRect(0, 0, 4, _rowH, ds.driver.team.teamColor);

    // Right-side stripe for selected row (team color extension)
    if (selected)
    {
        _rowSprite->fillRect(UI::SAFE_W - 4, 0, 4, _rowH, ds.driver.team.teamColor);
    }

    // Position - use DATA_ACCENT for selected
    _rowSprite->setTextDatum(middle_left);
    if (selected)
    {
        _rowSprite->setFont(UI::Fonts::DATA_ACCENT);
        _rowSprite->setTextColor(ds.driver.team.teamColor);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
    }
    _rowSprite->drawNumber(ds.position, COL_POS, _rowH / 2);

    // Driver name
    _rowSprite->setTextDatum(middle_left);
    if (selected)
    {
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(UI::COL_TEXT);
        _rowSprite->drawString(ds.driver.lastName, COL_NAME, _rowH / 2);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
        _rowSprite->drawString(ds.driver.acronym, COL_NAME, _rowH / 2);
    }

    // Team name
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(selected ? UI::COL_TEXT_DIM : dimCol(UI::COL_TEXT_DIM, brightness));
    _rowSprite->drawString(ds.driver.team.name, COL_TEAM, _rowH / 2);

    // Points (right-aligned)
    _rowSprite->setTextDatum(middle_right);
    if (selected)
    {
        _rowSprite->setFont(UI::Fonts::DATA_ACCENT);
        _rowSprite->setTextColor(ptsCol);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(ptsCol);
    }
    _rowSprite->drawNumber(ds.points, COL_PTS, _rowH / 2);
}

void DriverStandingsView::drawFooter()
{
    if (cache->driverStandings.empty() || _cursor >= (int)cache->driverStandings.size())
        return;

    const auto &sel = cache->driverStandings[_cursor];
    const auto &leader = cache->driverStandings[0];
    int gap = leader.points - sel.points;

    char buf[48];
    uint32_t color;
    if (_cursor == 0)
    {
        color = sel.driver.team.teamColor;
        snprintf(buf, sizeof(buf), "CHAMPION: %s", sel.driver.lastName);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        snprintf(buf, sizeof(buf), "Gap to P1: +%d pts", gap);
    }

    drawFooterText(buf, UI::SCREEN_W - 10, UI::FOOTER_Y + (UI::FOOTER_H / 2), color, UI::FONT_SMALL);
}
