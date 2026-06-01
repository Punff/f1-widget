#include "ConstructorStandingsView.h"
#include "../DisplayManager.h"
#include "../../data/DataCache.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

// Column offsets
static constexpr int COL_POS = 15;
static constexpr int COL_NAME = 65;
static constexpr int COL_PTS = 465;

ConstructorStandingsView::ConstructorStandingsView(LGFX *tft, DisplayManager *dm)
    : ScrollListView(tft, dm, 46, 5, 2) // RowH=46px, 5 rows, center at index 2
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
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);

    _tft->setTextDatum(middle_left);
    _tft->drawString("#", COL_POS, 42);
    _tft->drawString("TEAM", COL_NAME, 42);

    _tft->setTextDatum(middle_right);
    _tft->drawString("PTS", COL_PTS, 42);
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
    _rowSprite->drawString(posBuf, COL_POS, _rowH / 2);

    // Team name
    _rowSprite->setTextColor(nameCol);
    _rowSprite->drawString(cs.team.name, COL_NAME, _rowH / 2);

    // Points — team color
    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(tc);
    char ptsStr[16];
    if (cs.points == (int)cs.points) snprintf(ptsStr, sizeof(ptsStr), "%d", (int)cs.points);
    else snprintf(ptsStr, sizeof(ptsStr), "%.1f", cs.points);
    _rowSprite->drawString(ptsStr, COL_PTS, _rowH / 2);
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
        color = ((tc & 0xF800) << 8) | ((tc & 0x07E0) << 5) | ((tc & 0x001F) << 3);
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

void ConstructorStandingsView::onTurnRight()
{
    ScrollListView::onTurnRight();
}

void ConstructorStandingsView::onTurnLeft()
{
    ScrollListView::onTurnLeft();
}

void ConstructorStandingsView::onLongPress()
{
    _dm->returnToMenu();
}
