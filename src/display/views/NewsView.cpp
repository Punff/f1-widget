#include "NewsView.h"
#include "../DisplayManager.h"

NewsView::NewsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm) {}

void NewsView::onEnter()
{
    _tft->fillScreen(UI::COL_BG);
    drawNewsHeader();
    drawNewsContent();
}

void NewsView::render()
{
    // Static view, no updates needed
}

void NewsView::drawNewsHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // Title
    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setTextSize(2);
    _tft->drawString("F1", 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->drawString("LATEST NEWS", 60, 10);

    // Red separator
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);
}

void NewsView::drawNewsContent()
{
    // Placeholder content
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_TEXT_DIM);
    _tft->setTextSize(1);

    int centerY = UI::HEADER_H + (UI::CONTENT_H / 2);

    _tft->drawString("News Feed Coming Soon", UI::SCREEN_W / 2, centerY - 20);
    _tft->drawString("Stay tuned for F1 updates!", UI::SCREEN_W / 2, centerY + 20);
}
