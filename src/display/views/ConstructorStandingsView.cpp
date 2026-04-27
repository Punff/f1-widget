#include "ConstructorStandingsView.h"
#include "../../data/DataCache.h"
#include "../EncoderWidget.h"

extern DataCache *cache;

ConstructorStandingsView::ConstructorStandingsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _cursor(0), _scrollOffset(0) {}

// ── Lifecycle ─────────────────────────────────────────────────────────────

void ConstructorStandingsView::onEnter()
{
    _cursor = 0;
    _tft->fillScreen(TFT_BLACK);
    _updateScrollOffset();
    _fullRedraw();
}

void ConstructorStandingsView::render()
{
    _fullRedraw();
}

// ── Rendering Logic ───────────────────────────────────────────────────────

void ConstructorStandingsView::_updateScrollOffset()
{
    // Locked Selection: Selection is always at CENTER_ROW (3)
    _scrollOffset = _cursor - CENTER_ROW;
}

void ConstructorStandingsView::_renderHeader()
{
    _tft->fillRect(0, 0, SAFE_W, 40, 0x0000);

    _tft->setTextDatum(middle_left);
    _tft->setTextColor(0xBDF7); // Soft F1 Grey
    _tft->setTextSize(1);
    _tft->drawString("FORMULA 1 WORLD CHAMPIONSHIP", 10, 12);

    _tft->setTextColor(TFT_WHITE);
    _tft->setTextSize(1);
    _tft->drawString("CONSTRUCTOR STANDINGS", 10, 26);

    // F1 Red Accent Line
    _tft->drawFastHLine(0, 39, SAFE_W, 0xE80020);
}

void ConstructorStandingsView::_renderRow(int row, int idx, int rowY)
{
    bool selected = (row == CENTER_ROW);
    uint32_t bg24 = selected ? 0x1A1A1A : 0x000000;
    uint16_t bg16 = _tft->color24to16(bg24);

    // Bounds Check
    if (idx < 0 || idx >= (int)cache->constructorStandings.size())
    {
        _tft->fillRect(0, rowY, SAFE_W, ROW_H, TFT_BLACK);
        return;
    }

    const auto &cs = cache->constructorStandings[idx];

    // Background and Team Accent
    _tft->fillRect(0, rowY, SAFE_W, ROW_H, bg16);
    _tft->fillRect(0, rowY, 4, ROW_H, cs.team.teamColor);

    // Dimming logic for non-selected rows
    int dist = abs(row - CENTER_ROW);
    float brightness = (dist == 0) ? 1.0f : (1.0f - (dist * 0.22f));
    uint16_t textCol = selected ? TFT_WHITE : _tft->color24to16(_dimCol(0xFFFFFF, brightness));

    _tft->setTextDatum(middle_left);
    _tft->setTextColor(textCol, bg16);

    // Position
    _tft->setTextSize(1);
    _tft->drawNumber(cs.position, 18, rowY + ROW_H / 2);

    // Team Name
    _tft->setTextSize(selected ? 2 : 1);
    _tft->drawString(cs.team.name, 65, rowY + ROW_H / 2);

    // Points
    _tft->setTextDatum(middle_right);
    uint16_t ptsCol = selected ? cs.team.teamColor : textCol;
    _tft->setTextColor(ptsCol, bg16);
    _tft->drawNumber(cs.points, SAFE_W - 15, rowY + ROW_H / 2);
}

void ConstructorStandingsView::_renderConnector()
{
    // Aligns visually with the hardware encoder knob
    int cy = 40 + (CENTER_ROW * ROW_H) + (ROW_H / 2);

    if (_cursor < (int)cache->constructorStandings.size())
    {
        uint16_t teamCol = cache->constructorStandings[_cursor].team.teamColor;

        int x1 = SAFE_W;
        int x2 = EncoderWidget::CX - EncoderWidget::RADIUS - 5;

        _tft->drawFastHLine(x1, cy, x2 - x1, teamCol);
        _tft->fillTriangle(x1 + 6, cy, x1, cy - 6, x1, cy + 6, teamCol);
    }
}

void ConstructorStandingsView::_renderFooter()
{
    _tft->fillRect(0, 296, SAFE_W, 24, 0x0000);
    if (cache->constructorStandings.empty() || _cursor >= (int)cache->constructorStandings.size())
        return;

    const auto &sel = cache->constructorStandings[_cursor];
    const auto &leader = cache->constructorStandings[0];
    int gap = leader.points - sel.points;

    _tft->setTextSize(1);
    _tft->setTextDatum(middle_right);

    char buf[32];
    if (_cursor == 0)
    {
        _tft->setTextColor(sel.team.teamColor);
        snprintf(buf, sizeof(buf), "CHAMPIONSHIP LEADER");
    }
    else
    {
        _tft->setTextColor(0xBDF7); // Grey
        snprintf(buf, sizeof(buf), "GAP TO P1: +%d PTS", gap);
    }
    _tft->drawString(buf, SAFE_W - 10, 296 + 12);
}

void ConstructorStandingsView::_fullRedraw()
{
    if (cache->constructorStandings.empty())
    {
        _tft->drawCenterString("WAITING FOR DATA...", 200, 160);
        return;
    }

    _updateScrollOffset();
    _tft->startWrite();

    _renderHeader();

    for (int row = 0; row < ROWS_VISIBLE; row++)
    {
        _renderRow(row, _scrollOffset + row, 40 + (row * ROW_H));
    }

    _renderConnector();
    _renderFooter();

    _tft->endWrite();
}

// ── Input Handling ────────────────────────────────────────────────────────

void ConstructorStandingsView::onTurnRight()
{
    if (_cursor < (int)cache->constructorStandings.size() - 1)
    {
        _cursor++;
        _fullRedraw();
    }
}

void ConstructorStandingsView::onTurnLeft()
{
    if (_cursor > 0)
    {
        _cursor--;
        _fullRedraw();
    }
}

// ── Helpers ──────────────────────────────────────────────────────────────

uint32_t ConstructorStandingsView::_dimCol(uint32_t col, float b) const
{
    uint8_t r = ((col >> 16) & 0xFF) * b;
    uint8_t g = ((col >> 8) & 0xFF) * b;
    uint8_t bv = (col & 0xFF) * b;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bv;
}