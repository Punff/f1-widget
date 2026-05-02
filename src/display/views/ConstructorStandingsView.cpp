#include "ConstructorStandingsView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

ConstructorStandingsView::ConstructorStandingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 42, 6, 3) // RowH=42px, 6 rows (not 7!), center at index 3
{
}

int ConstructorStandingsView::dataSize() const
{
    return cache->constructorStandings.size();
}

void ConstructorStandingsView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title - use centralized fonts
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("CONSTRUCTOR STANDINGS", 75, 12);

    // Red separator
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

    // Column headers
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("#", 20, 34);
    _tft->drawString("TEAM", 65, 34);
    _tft->drawString("PTS", 430, 34);
}

void ConstructorStandingsView::drawRow(int dataIdx, bool selected, int dist)
{
    const auto &cs = cache->constructorStandings[dataIdx];

    // Team color bar on left
    _rowSprite->fillRect(0, 0, 4, _rowH, cs.team.teamColor);

    // Right-side stripe for selected row
    if (selected)
    {
        _rowSprite->fillRect(UI::SAFE_W - 4, 0, 4, _rowH, cs.team.teamColor);
    }

    // Calculate colors
    float brightness = rowBrightness(dist);
    uint32_t textCol = selected ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);
    uint32_t ptsCol = selected ? cs.team.teamColor : dimCol(UI::COL_TEXT, brightness);

    // Position
    _rowSprite->setTextDatum(middle_left);
    if (selected)
    {
        _rowSprite->setFont(UI::Fonts::DATA_ACCENT);
        _rowSprite->setTextColor(cs.team.teamColor);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
    }
    _rowSprite->drawNumber(cs.position, 20, _rowH / 2);

    // Team name
    _rowSprite->setTextDatum(middle_left);
    if (selected)
    {
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(UI::COL_TEXT);
    }
    else
    {
        _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
        _rowSprite->setTextColor(textCol);
    }
    _rowSprite->drawString(cs.team.name, 65, _rowH / 2);

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
    _rowSprite->drawNumber(cs.points, 460, _rowH / 2);
}

void ConstructorStandingsView::drawFooter()
{
    if (cache->constructorStandings.empty() || _cursor >= (int)cache->constructorStandings.size())
        return;

    const auto &sel = cache->constructorStandings[_cursor];
    const auto &leader = cache->constructorStandings[0];
    int gap = leader.points - sel.points;

    char buf[48];
    uint32_t color;
    if (_cursor == 0)
    {
        color = sel.team.teamColor;
        snprintf(buf, sizeof(buf), "CHAMPION: %s", sel.team.name);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        snprintf(buf, sizeof(buf), "Gap to P1: +%d pts", gap);
    }

    // Draw footer directly (no optimization)
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);
    _tft->setTextColor(color);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextDatum(middle_right);
    _tft->drawString(buf, UI::SCREEN_W - 10, UI::FOOTER_Y + (UI::FOOTER_H / 2));
}
