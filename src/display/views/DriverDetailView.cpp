#include "DriverDetailView.h"
#include "../DisplayManager.h"
#include "../../data/DataCache.h"
#include "../../time/TimeManager.h"
#include "../../../include/UI_Fonts.h"
#include <time.h>

extern DataCache *cache;
extern TimeManager *timeMgr;

static const char *monthName(int m)
{
    static const char *names[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (m < 1 || m > 12)
        return "?";
    return names[m - 1];
}

DriverDetailView::DriverDetailView(LGFX *tft, DisplayManager *dm, int driverIdx)
    : _tft(tft), _dm(dm), _driverIdx(driverIdx) {}

void DriverDetailView::onEnter() {}

void DriverDetailView::render()
{
    if (_driverIdx < 0 || _driverIdx >= (int)cache->driverStandings.size())
    {
        _dm->returnToPrevious();
        return;
    }
    drawCard();
}

void DriverDetailView::onTurnLeft()
{
    if (_driverIdx > 0)
    {
        _driverIdx--;
        drawCard();
    }
}

void DriverDetailView::onTurnRight()
{
    if (_driverIdx < (int)cache->driverStandings.size() - 1)
    {
        _driverIdx++;
        drawCard();
    }
}

void DriverDetailView::onLongPress()
{
    _dm->returnToPrevious();
}

// Format "1997-09-30" → "30 Sep 1997", extracts year
static int parseDOB(const char *iso, char *out, size_t outSize)
{
    if (!iso || !iso[0])
    {
        out[0] = 0;
        return 0;
    }
    int y = 0, m = 0, d = 0;
    if (sscanf(iso, "%d-%d-%d", &y, &m, &d) >= 3)
        snprintf(out, outSize, "%d %s %d", d, monthName(m), y);
    else
        strlcpy(out, iso, outSize);
    return y;
}

void DriverDetailView::drawCard()
{
    const auto &ds = cache->driverStandings[_driverIdx];
    uint32_t team888 = UI::rgb565to888(ds.driver.team.teamColor);
    uint16_t team565 = ds.driver.team.teamColor;
    int cx = UI::SCREEN_W / 2;

    static constexpr int LEFT_W = 170;
    static constexpr int RIGHT_X = 196;

    _tft->fillScreen(UI::COL_BG);
    _tft->waitDMA();

    _dm->header()->draw("DRIVER INFO");

    // ── Left panel: team-colored poster with big number ──
    _tft->fillRect(0, UI::HEADER_H, LEFT_W, UI::FOOTER_Y - UI::HEADER_H, team565);
    _tft->setTextDatum(middle_center);

    // Number: huge font centered in left panel
    char numBuf[4];
    snprintf(numBuf, sizeof(numBuf), "%d", ds.driver.number);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(numBuf, LEFT_W / 2, (UI::HEADER_H + UI::FOOTER_Y) / 2 - 14);

    // Acronym below number
    _tft->setTextColor(UI::COL_BG);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(ds.driver.acronym, LEFT_W / 2, (UI::HEADER_H + UI::FOOTER_Y) / 2 + 20);

    // Small team name at bottom of left panel
    _tft->setTextColor(UI::COL_BG);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(ds.driver.team.name, LEFT_W / 2, UI::FOOTER_Y - 10);

    // ── Right panel: driver info ──
    int ry = UI::HEADER_H + 18;

    // Full name
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(ds.driver.fullName, RIGHT_X, ry);
    ry += 24;

    // Acronym + Nationality
    _tft->setTextColor(team888);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(ds.driver.acronym, RIGHT_X, ry);
    if (ds.driver.nationality[0])
    {
        _tft->setTextColor(UI::COL_MUTED);
        char natBuf[48];
        snprintf(natBuf, sizeof(natBuf), "   ·   %s", ds.driver.nationality);
        int acroW = _tft->textWidth(ds.driver.acronym);
        _tft->drawString(natBuf, RIGHT_X + acroW, ry);
    }
    ry += 40;

    // Team name with color dot (middle_left for vertical alignment)
    _tft->setTextDatum(middle_left);
    _tft->fillCircle(RIGHT_X + 6, ry, 5, team565);
    _tft->setTextColor(UI::COL_TEXT_DIM);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(ds.driver.team.name, RIGHT_X + 20, ry);
    _tft->setTextDatum(top_left);
    ry += 30;

    // Thin separator
    _tft->fillRect(RIGHT_X, ry, UI::SCREEN_W - RIGHT_X - 10, 1, 0x333333);
    ry += 12;

    // ── Stats ──
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->setTextDatum(top_left);
    static constexpr int V1 = RIGHT_X;        // col1 label
    static constexpr int VV1 = RIGHT_X + 70;  // col1 value
    static constexpr int V2 = RIGHT_X + 160;  // col2 label
    static constexpr int VV2 = RIGHT_X + 230; // col2 value

    // Born
    char dobStr[24] = "";
    int birthYear = parseDOB(ds.driver.dateOfBirth, dobStr, sizeof(dobStr));
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Born", V1, ry);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->drawString(dobStr, VV1, ry);
    ry += 26;

    // Age
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Age", V1, ry);
    _tft->setTextColor(UI::COL_TEXT);
    if (birthYear > 0)
    {
        time_t now = timeMgr->getLocalTime();
        struct tm lt;
        localtime_r(&now, &lt);
        int yr = lt.tm_year + 1900;
        char ageBuf[8];
        snprintf(ageBuf, sizeof(ageBuf), "%d", yr - birthYear);
        _tft->drawString(ageBuf, VV1, ry);
    }
    ry += 28;

    // Two-column row: Pos + Pts
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Pos", V1, ry);
    _tft->setTextColor(UI::COL_TEXT);
    char posBuf[16];
    snprintf(posBuf, sizeof(posBuf), "%d%s", ds.position,
             ds.position == 1 ? "st" : ds.position == 2 ? "nd"
                                   : ds.position == 3   ? "rd"
                                                        : "th");
    _tft->drawString(posBuf, VV1, ry);

    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Pts", V2, ry);
    _tft->setTextColor(team888);
    char ptsStr[16];
    if (ds.points == (int)ds.points)
        snprintf(ptsStr, sizeof(ptsStr), "%d", (int)ds.points);
    else
        snprintf(ptsStr, sizeof(ptsStr), "%.1f", ds.points);
    _tft->drawString(ptsStr, VV2, ry);
    ry += 24;

    // Two-column row: Wins + Gap
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Wins", V1, ry);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->drawNumber(ds.wins, VV1, ry);

    if (cache->driverStandings.size() > 0)
    {
        float leaderPts = cache->driverStandings[0].points;
        float gap = leaderPts - ds.points;
        _tft->setTextColor(UI::COL_MUTED);
        _tft->drawString("Gap", V2, ry);
        _tft->setTextColor(UI::COL_TEXT_DIM);
        if (_driverIdx == 0)
        {
            _tft->drawString("--", VV2, ry);
        }
        else if (gap == (int)gap)
        {
            char gb[16];
            snprintf(gb, sizeof(gb), "+%d", (int)gap);
            _tft->drawString(gb, VV2, ry);
        }
        else
        {
            char gb[16];
            snprintf(gb, sizeof(gb), "+%.1f", gap);
            _tft->drawString(gb, VV2, ry);
        }
    }

    // ── Footer ──
    _dm->footer()->markDirty();
    _dm->footer()->draw();
    char fBuf[48];
    snprintf(fBuf, sizeof(fBuf), "DRV \xc2\xb7 %d / %zu", _driverIdx + 1, cache->driverStandings.size());
    _dm->footer()->drawCenter(fBuf, UI::COL_MUTED);
}
