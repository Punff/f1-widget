#include "ConstructorStandingsView.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

// Column offsets
static constexpr int COL_POS = 15;
static constexpr int COL_NAME = 65;
static constexpr int COL_PTS = 465;

ConstructorStandingsView::ConstructorStandingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 48, 5, 2) // RowH=48px, 5 rows, center at index 2
{
}

int ConstructorStandingsView::dataSize() const
{
    return cache->constructorStandings.size();
}

void ConstructorStandingsView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title
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
    _tft->drawString("#", COL_POS, 34);
    _tft->drawString("TEAM", COL_NAME, 34);
    _tft->setTextDatum(top_right);
    _tft->drawString("PTS", COL_PTS, 34);
}

void ConstructorStandingsView::drawRow(int dataIdx, bool selected, int dist)
{
    (void)dist;
    if (dataIdx < 0 || dataIdx >= (int)cache->constructorStandings.size()) return;
    const auto &cs = cache->constructorStandings[dataIdx];
    uint16_t tc = cs.team.teamColor;

    // Team color bar on left (always full brightness)
    _rowSprite->fillRect(0, 0, 4, _rowH, tc);

    if (selected) {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, tc);
    }

    // Position
    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(UI::COL_TEXT);
    _rowSprite->drawNumber(cs.position, COL_POS, _rowH / 2);

    // Team name
    _rowSprite->drawString(cs.team.name, COL_NAME, _rowH / 2);

    // Points
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(UI::COL_TEXT);
    _rowSprite->drawNumber(cs.points, COL_PTS, _rowH / 2);
}

void ConstructorStandingsView::drawFooter()
{
    if (cache->constructorStandings.empty() || _cursor >= (int)cache->constructorStandings.size())
        return;

    const auto &sel = cache->constructorStandings[_cursor];
    const auto &leader = cache->constructorStandings[0];
    int gap = leader.points - sel.points;

    char buf[64];
    uint32_t color;
    if (_cursor == 0)
    {
        color = sel.team.teamColor;
        snprintf(buf, sizeof(buf), "LEADER: %s", sel.team.name);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        snprintf(buf, sizeof(buf), "GAP TO P1: +%d PTS", gap);
    }

    drawFooterText(buf, UI::SCREEN_W - 10, UI::FOOTER_Y + (UI::FOOTER_H / 2), color, UI::FONT_SMALL);
}
