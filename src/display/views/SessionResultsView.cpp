#include "SessionResultsView.h"
#include "../DisplayManager.h"
#include "../../api/APIClient.h"
#include "../../../include/UI_Fonts.h"

extern DataCache *cache;

// Column offsets (using shared UI constants)

SessionResultsView::SessionResultsView(LGFX *tft, DisplayManager *dm, int round,
                                       const char *officialName, const char *circuitName,
                                       const char *sessionName)
    : ScrollListView(tft, dm, 43, 5, 2, 14),
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
        _tft->setTextColor(UI::COL_ACCENT);
        _tft->setFont(UI::Fonts::HEADER_BIG);
        _tft->drawString(buf, UI::SCREEN_W / 2, 120);

        _tft->setTextColor(UI::COL_TEXT);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString(_sessionName, UI::SCREEN_W / 2, 152);

        _tft->drawFastHLine(UI::SCREEN_W / 2 - 20, 178, 40, UI::COL_ACCENT);

        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString("LOADING", UI::SCREEN_W / 2, 200);

        APIClient api(cache);
        _fetchFailed = !api.fetchSessionResults(_meetingRound, _sessionName, _results);

        if (_fetchFailed)
            Serial.printf("[SR] Fetch failed for Rd %d %s\n", _meetingRound, _sessionName);
    }

    fullRedraw();
}

void SessionResultsView::drawHeader()
{
    char roundStr[8];
    snprintf(roundStr, sizeof(roundStr), "R%02d", _meetingRound);
    _dm->header()->draw(_sessionName, _circuitName, roundStr);

    _tft->setTextDatum(middle_left);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    int chY = UI::HEADER_H + _colH / 2;
    _tft->drawString("POS", UI::COL_POS, chY);
    _tft->drawString("DRIVER", UI::COL_PRIMARY, chY);
    _tft->drawString("TEAM", UI::COL_SECONDARY, chY);

    _tft->setTextDatum(middle_right);
    _tft->drawString("TIME", UI::COL_VALUE_R, chY);
    _tft->drawString("PTS", UI::COL_END_R, chY);
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
    uint16_t teamColor = 0x7BEF; // Default grey in 565
    for (const auto &ds : cache->driverStandings)
    {
        if (strcmp(ds.driver.acronym, sr.driverCode) == 0)
        {
            driver = &ds.driver;
            teamColor = ds.driver.team.teamColor;
            break;
        }
    }
    if (teamColor == 0x7BEF) {
        for (const auto &cs : cache->constructorStandings) {
            if (strcmp(cs.team.name, sr.constructorName) == 0) {
                teamColor = (uint16_t)cs.team.teamColor;
                break;
            }
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
    _rowSprite->drawNumber(sr.position, UI::COL_POS, _rowH / 2);

    if (sr.isFastestLap)
        _rowSprite->fillCircle(UI::COL_PRIMARY - 9, _rowH / 2, 3, UI::COL_F1_PURPLE);

    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(driver ? driver->acronym : sr.driverCode, UI::COL_PRIMARY, _rowH / 2);

    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(sr.constructorName, UI::COL_SECONDARY, _rowH / 2);

    _rowSprite->setTextDatum(middle_right);
    _rowSprite->setFont(UI::Fonts::LABEL_SMALL);
    _rowSprite->setTextColor(dim);
    _rowSprite->drawString(sr.timeOrStatus, UI::COL_VALUE_R, _rowH / 2);

    _rowSprite->setFont(UI::Fonts::BODY_MAIN);
    _rowSprite->setTextColor(teamColor);
    char ptsStr[16];
    if (sr.points == (int)sr.points) snprintf(ptsStr, sizeof(ptsStr), "%d", (int)sr.points);
    else snprintf(ptsStr, sizeof(ptsStr), "%.1f", sr.points);
    _rowSprite->drawString(ptsStr, UI::COL_END_R, _rowH / 2);
}

void SessionResultsView::drawFooter()
{
    _dm->footer()->draw();

    if (_results.empty()) return;

    const auto &sel = _results[_cursor >= (int)_results.size() ? 0 : _cursor];
    const auto &leader = _results[0];
    float gap = leader.points - sel.points;

    char buf[48];
    uint32_t color;
    if (_cursor == 0)
    {
        color = UI::COL_MUTED;
        const char *winnerCode = "";
        for (const auto &ds : cache->driverStandings)
        {
            if (strcmp(ds.driver.acronym, leader.driverCode) == 0)
            {
                winnerCode = ds.driver.acronym;
                uint16_t tc = ds.driver.team.teamColor;
                color = UI::rgb565to888(tc);
                break;
            }
        }
        snprintf(buf, sizeof(buf), "SR \xc2\xb7 P1: %s", winnerCode);
    }
    else
    {
        color = UI::COL_TEXT_DIM;
        if (gap == (int)gap) snprintf(buf, sizeof(buf), "SR \xc2\xb7 Gap: +%d", (int)gap);
        else snprintf(buf, sizeof(buf), "SR \xc2\xb7 Gap: +%.1f", gap);
    }

    _dm->footer()->drawCenter(buf, color);
}

void SessionResultsView::onPress()
{
}

void SessionResultsView::onLongPress()
{
    _dm->returnToPrevious();
}
