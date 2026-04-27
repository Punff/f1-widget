#include "DriverStandingsView.h"
#include "../DisplayManager.h"
#include "../EncoderWidget.h"
#include "../../data/DataCache.h"

extern DataCache *cache;

DriverStandingsView::DriverStandingsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _cursor(0), _scrollOffset(0) {}

void DriverStandingsView::onEnter()
{
    _cursor = 0;
    _tft->fillScreen(TFT_BLACK);
    _fullRedraw();
}

void DriverStandingsView::render()
{
    _fullRedraw();
}

void DriverStandingsView::_updateScrollOffset()
{
    // Selection is always at CENTER_ROW
    _scrollOffset = _cursor - CENTER_ROW;
}

void DriverStandingsView::_renderRow(int row, int idx, int rowY)
{
    bool selected = (row == CENTER_ROW);
    uint16_t bg = selected ? _tft->color24to16(0x1A1A1A) : 0x0000;

    if (idx < 0 || idx >= (int)cache->driverStandings.size())
    {
        _tft->fillRect(0, rowY, SAFE_W, ROW_H, TFT_BLACK);
        return;
    }

    const auto &ds = cache->driverStandings[idx];
    _tft->fillRect(0, rowY, SAFE_W, ROW_H, bg);
    _tft->fillRect(0, rowY, ACCENT_W, ROW_H, ds.driver.team.teamColor);

    int dist = abs(row - CENTER_ROW);
    float brightness = (dist == 0) ? 1.0f : (1.0f - (dist * 0.22f));
    uint16_t textCol = selected ? TFT_WHITE : _tft->color24to16(_dimCol(0xFFFFFF, brightness));

    _tft->setTextDatum(middle_left);
    _tft->setTextColor(textCol, bg);

    // This now shows the correct sorted position (1, 2, 3...)
    _tft->setTextSize(1);
    _tft->drawNumber(ds.position, COL_POS, rowY + ROW_H / 2);

    _tft->setTextSize(selected ? 2 : 1);
    _tft->drawString(ds.driver.lastName, COL_NAME, rowY + ROW_H / 2);

    _tft->setTextDatum(middle_right);
    _tft->setTextColor(selected ? ds.driver.team.teamColor : textCol, bg);
    _tft->drawNumber(ds.points, COL_PTS, rowY + ROW_H / 2);
}

void DriverStandingsView::_renderConnector()
{
    int cy = START_Y + (CENTER_ROW * ROW_H) + (ROW_H / 2);
    if (_cursor < (int)cache->driverStandings.size())
    {
        uint16_t teamCol = cache->driverStandings[_cursor].driver.team.teamColor;

        int x1 = SAFE_W;
        int x2 = EncoderWidget::CX - EncoderWidget::RADIUS - 5;

        // Pointer aligned with encoder wheel
        _tft->drawFastHLine(x1, cy, x2 - x1, teamCol);
        _tft->fillTriangle(x1 + 5, cy, x1, cy - 5, x1, cy + 5, teamCol);
    }
}

void DriverStandingsView::_renderHeader()
{
    _tft->fillRect(0, 0, SAFE_W, HEADER_H, 0x0000);

    // F1 Style "Championship" Header
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(0xBDF7); // Soft Grey
    _tft->setTextSize(1);
    _tft->drawString("FORMULA 1 WORLD CHAMPIONSHIP", 10, 10);

    _tft->setTextColor(TFT_WHITE);
    _tft->setTextSize(1);
    _tft->drawString("DRIVER STANDINGS", 10, 22);

    // Accent line
    _tft->drawFastHLine(0, HEADER_H - 1, SAFE_W, 0x4208);
}

void DriverStandingsView::_renderFooter()
{
    _tft->fillRect(0, FOOTER_Y, SAFE_W, FOOTER_H, 0x0000);
    if (_cursor >= (int)cache->driverStandings.size())
        return;

    const auto &sel = cache->driverStandings[_cursor];
    const auto &leader = cache->driverStandings[0];
    int gap = leader.points - sel.points;

    _tft->setTextSize(1);
    _tft->setTextDatum(middle_right);

    char buf[32];
    if (_cursor == 0)
    {
        _tft->setTextColor(sel.driver.team.teamColor);
        snprintf(buf, sizeof(buf), "CHAMPIONSHIP LEADER");
    }
    else
    {
        _tft->setTextColor(0xBDF7);
        snprintf(buf, sizeof(buf), "GAP TO P1: +%d PTS", gap);
    }

    _tft->drawString(buf, COL_PTS, FOOTER_Y + FOOTER_H / 2);
}
void DriverStandingsView::_fullRedraw()
{
    if (cache->driverStandings.empty())
    {
        _tft->drawCenterString("SYNCING DATA...", 200, 160);
        return;
    }

    _updateScrollOffset();
    _tft->startWrite();
    _renderHeader();
    for (int row = 0; row < ROWS_VISIBLE; row++)
    {
        _renderRow(row, _scrollOffset + row, START_Y + (row * ROW_H));
    }
    _renderConnector();
    _renderFooter();
    _tft->endWrite();
}

void DriverStandingsView::onTurnRight()
{
    if (_cursor < (int)cache->driverStandings.size() - 1)
    {
        _cursor++;
        _fullRedraw();
    }
}

void DriverStandingsView::onTurnLeft()
{
    if (_cursor > 0)
    {
        _cursor--;
        _fullRedraw();
    }
}

uint32_t DriverStandingsView::_dimCol(uint32_t col, float b) const
{
    uint8_t r = ((col >> 16) & 0xFF) * b;
    uint8_t g = ((col >> 8) & 0xFF) * b;
    uint8_t bv = (col & 0xFF) * b;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bv;
}