#include "ConstructorStandingsView.h"
#include "../DisplayManager.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

// Column offsets (using shared UI constants)

ConstructorStandingsView::ConstructorStandingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 43, 5, 2, 14) // RowH=43px, 5 rows, center at index 2
{
}

int ConstructorStandingsView::dataSize() const
{
    return cache->constructorStandings.size();
}

void ConstructorStandingsView::drawHeader()
{
    _dm->header()->draw("CONSTRUCTOR STANDINGS");

    // Column headers
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    int chY = UI::HEADER_H + _colH / 2;
    _tft->drawString("#", UI::COL_POS, chY);
    _tft->drawString("TEAM", UI::COL_PRIMARY, chY);

    _tft->setTextDatum(middle_right);
    _tft->drawString("PTS", UI::COL_END_R, chY);
}

void ConstructorStandingsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (dataIdx < 0 || dataIdx >= (int)cache->constructorStandings.size())
        return;
    const auto &cs = cache->constructorStandings[dataIdx];
    uint16_t tc = cs.team.teamColor;

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
    snprintf(posBuf, sizeof(posBuf), "%2d", cs.position);
    _rowSprite->drawString(posBuf, UI::COL_POS, _rowH / 2);

    // Team name
    _rowSprite->setTextColor(nameCol);
    _rowSprite->drawString(cs.team.name, UI::COL_PRIMARY, _rowH / 2);

    // Points — team color
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(tc);
    char ptsStr[16];
    if (cs.points == (int)cs.points) snprintf(ptsStr, sizeof(ptsStr), "%d", (int)cs.points);
    else snprintf(ptsStr, sizeof(ptsStr), "%.1f", cs.points);
    _rowSprite->drawString(ptsStr, UI::COL_END_R, _rowH / 2);
}

void ConstructorStandingsView::drawFooter()
{
    _dm->footer()->draw();

    if (cache->constructorStandings.empty() || _cursor >= (int)cache->constructorStandings.size())
        return;

    const auto &sel = cache->constructorStandings[_cursor];
    const auto &leader = cache->constructorStandings[0];
    float gap = leader.points - sel.points;

    char buf[48];
    uint32_t color;
    if (_cursor == 0)
    {
        uint16_t tc = sel.team.teamColor;
        color = UI::rgb565to888(tc);
        snprintf(buf, sizeof(buf), "CS \xc2\xb7 P1: %s", sel.team.name);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        if (gap == (int)gap) snprintf(buf, sizeof(buf), "CS \xc2\xb7 Gap: +%d", (int)gap);
        else snprintf(buf, sizeof(buf), "CS \xc2\xb7 Gap: +%.1f", gap);
    }

    _dm->footer()->drawCenter(buf, color);
}

void ConstructorStandingsView::onLongPress()
{
    _dm->returnToMenu();
}
