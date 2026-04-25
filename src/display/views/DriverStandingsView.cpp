#include "DriverStandingsView.h"
#include "../DisplayManager.h"
#include "../../data/DataCache.h"

extern DataCache *cache;

DriverStandingsView::DriverStandingsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _scrollOffset(0) {}

void DriverStandingsView::onEnter()
{
    _tft->fillScreen(TFT_BLACK);
    _scrollOffset = 0;
    render();
}

void DriverStandingsView::onExit()
{
    _tft->fillScreen(TFT_BLACK);
}

void DriverStandingsView::render()
{
    _tft->startWrite(); // Lock SPI for faster drawing
    _tft->fillScreen(TFT_BLACK);

    // Header
    _tft->fillRect(0, 0, 320, 35, 0x2104);
    _tft->setTextColor(TFT_WHITE);
    _tft->setTextSize(2);
    _tft->setTextDatum(textdatum_t::middle_left);
    _tft->drawString(" DRIVER STANDINGS", 10, 18);

    if (cache->driverCount == 0)
    {
        _tft->setTextSize(1);
        _tft->drawString("No data. Waiting for Sync...", 10, 60);
        _tft->endWrite();
        return;
    }

    const int colPos = 10;
    const int colCode = 45;
    const int colName = 95;
    const int colPts = 300;

    int startY = 40;
    int rowH = 28;
    int maxVisible = 7;

    for (int i = 0; i < maxVisible; i++)
    {
        int idx = i + _scrollOffset;
        if (idx >= cache->driverCount)
            break;

        DriverStanding &d = cache->drivers[idx];
        int y = startY + (i * rowH);
        int centerY = y + (rowH / 2);

        if (idx % 2 == 0)
        {
            _tft->fillRect(0, y, 320, rowH, 0x0841);
        }

        _tft->setTextSize(1);
        _tft->setTextDatum(textdatum_t::middle_left);

        // Position
        _tft->setTextColor(0xBDD7);
        _tft->drawNumber(d.position, colPos, centerY);

        // Code
        _tft->setTextColor(TFT_YELLOW);
        _tft->drawString(d.code, colCode, centerY);

        // Name
        _tft->setTextColor(TFT_WHITE);
        _tft->drawString(d.lastName, colName, centerY);

        // Points
        _tft->setTextDatum(textdatum_t::middle_right);
        _tft->drawNumber(d.points, colPts, centerY);
    }

    // Scrollbar
    int totalH = 196;
    _tft->drawFastVLine(318, startY, totalH, 0x4208);
    int thumbY = startY + ((_scrollOffset * totalH) / cache->driverCount);
    _tft->fillRect(317, thumbY, 3, 20, TFT_RED);

    _tft->endWrite();
}

void DriverStandingsView::onTurnRight()
{
    if (_scrollOffset + 7 < cache->driverCount)
    {
        _scrollOffset++;
        render();
    }
}

void DriverStandingsView::onTurnLeft()
{
    if (_scrollOffset > 0)
    {
        _scrollOffset--;
        render();
    }
}

void DriverStandingsView::onPress() {}
void DriverStandingsView::onLongPress() {}
void DriverStandingsView::onDoublePress() {}