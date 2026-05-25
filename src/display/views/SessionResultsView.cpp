#include "SessionResultsView.h"
#include "../DisplayManager.h"
#include "../../api/APIClient.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

static constexpr int COL_POS = 15;
static constexpr int COL_DRIVER = 62;
static constexpr int COL_TEAM = 155;
static constexpr int COL_TIME = 350;
static constexpr int COL_PTS = 465;

SessionResultsView::SessionResultsView(LGFX *tft, DisplayManager *dm, int round,
                                       const char *officialName, const char *circuitName,
                                       const char *sessionName)
    : ScrollListView(tft, dm, 48, 5, 2),
      _meetingRound(round),
      _fetched(false), _fetchFailed(false)
{
    strlcpy(_officialName, officialName, sizeof(_officialName));
    strlcpy(_circuitName, circuitName, sizeof(_circuitName));
    strlcpy(_sessionName, sessionName, sizeof(_sessionName));
}

int SessionResultsView::dataSize() const
{
    return _results.size();
}

void SessionResultsView::onEnter()
{
    _rowSprite = _dm->rowSprite();
    _tft->fillScreen(UI::COL_BG);

    if (!_fetched)
    {
        _fetched = true;

        _tft->setTextDatum(middle_center);

        char buf[48];
        snprintf(buf, sizeof(buf), "R%02d", _meetingRound);
        _tft->setTextColor(UI::COL_F1_RED);
        _tft->setFont(UI::Fonts::HEADER_BIG);
        _tft->drawString(buf, UI::SCREEN_W / 2, 120);

        _tft->setTextColor(UI::COL_TEXT);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString(_sessionName, UI::SCREEN_W / 2, 152);

        _tft->drawFastHLine(UI::SCREEN_W / 2 - 20, 178, 40, UI::COL_F1_RED);

        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString("LOADING", UI::SCREEN_W / 2, 200);

        APIClient api(cache);
        _fetchFailed = !api.fetchSessionResults(_meetingRound, _sessionName, _results);

        if (_fetchFailed)
            Serial.printf("[SR] Fetch failed for Rd %d %s\n", _meetingRound, _sessionName);
    }

    fullRedraw();
    drawFooter();
}

void SessionResultsView::drawHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", _meetingRound);
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(roundStr, 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(_sessionName, 95, 12);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString("POS", COL_POS, 33);
    _tft->drawString("DRIVER", COL_DRIVER, 33);
    _tft->drawString("TEAM", COL_TEAM, 33);
    _tft->setTextDatum(top_right);
    _tft->drawString("TIME", COL_TIME, 33);
    _tft->drawString("PTS", COL_PTS, 33);

    _tft->fillRect(0, UI::HEADER_H - 2, UI::SCREEN_W, 2, UI::COL_F1_RED);
}

void SessionResultsView::drawRow(int dataIdx, bool selected, int dist)
{
    if (_fetchFailed)
    {
        _rowSprite->setTextDatum(middle_center);
        _rowSprite->setTextColor(UI::COL_MUTED);
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->drawString("Failed to load results", UI::SCREEN_W / 2, _rowH / 2);
        return;
    }

    if (dataIdx < 0 || dataIdx >= (int)_results.size()) return;
    const auto &sr = _results[dataIdx];

    const Driver *driver = nullptr;
    uint16_t teamColor = 0x4208;
    for (const auto &ds : cache->driverStandings)
    {
        if (strcmp(ds.driver.acronym, sr.driverCode) == 0)
        {
            driver = &ds.driver;
            teamColor = ds.driver.team.teamColor;
            break;
        }
    }

    uint32_t dim = selected ? UI::COL_TEXT : (dist < 2 ? UI::COL_TEXT_DIM : UI::COL_MUTED);

    _rowSprite->fillRect(0, 0, 4, _rowH, teamColor);

    if (selected)
    {
        _rowSprite->fillRect(4, 0, UI::SCREEN_W - 8, _rowH, UI::COL_BG_SEL);
        _rowSprite->fillRect(UI::SCREEN_W - 4, 0, 4, _rowH, teamColor);
    }

    _rowSprite->setTextDatum(middle_left);
    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(teamColor);
    _rowSprite->drawNumber(sr.position, COL_POS, _rowH / 2);

    if (sr.isFastestLap)
        _rowSprite->fillCircle(COL_DRIVER - 9, _rowH / 2, 3, UI::COL_F1_PURPLE);

    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(driver ? driver->acronym : sr.driverCode, COL_DRIVER, _rowH / 2);

    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(sr.constructorName, COL_TEAM, _rowH / 2);

    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(sr.timeOrStatus, COL_TIME, _rowH / 2);

    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(teamColor);
    _rowSprite->drawNumber(sr.points, COL_PTS, _rowH / 2);
}

void SessionResultsView::drawFooter()
{
    _dm->footer()->draw();

    if (_results.empty()) return;

    const auto &sel = _results[_cursor >= (int)_results.size() ? 0 : _cursor];
    const auto &leader = _results[0];
    int gap = leader.points - sel.points;

    char buf[48];
    uint32_t color;
    if (_cursor == 0)
    {
        color = 0x4208;
        const char *winnerCode = "";
        for (const auto &ds : cache->driverStandings)
        {
            if (strcmp(ds.driver.acronym, leader.driverCode) == 0)
            {
                winnerCode = ds.driver.acronym;
                color = ds.driver.team.teamColor;
                break;
            }
        }
        snprintf(buf, sizeof(buf), "SR \xc2\xb7 P1: %s", winnerCode);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        snprintf(buf, sizeof(buf), "SR \xc2\xb7 Gap: +%d", gap);
    }

    _dm->footer()->drawCenter(buf, color);
}

void SessionResultsView::onTurnRight() {
    ScrollListView::onTurnRight();
    drawFooter();
}

void SessionResultsView::onTurnLeft() {
    ScrollListView::onTurnLeft();
    drawFooter();
}

void SessionResultsView::onPress()
{
}

void SessionResultsView::onLongPress()
{
    _dm->returnToPrevious();
}
