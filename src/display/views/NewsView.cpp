#include "NewsView.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

NewsView::NewsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm) {}

void NewsView::onEnter()
{
    _tft->fillScreen(UI::COL_BG);
    drawNewsHeader();
    drawNewsContent();
}

void NewsView::render() {}

void NewsView::onLongPress() { _dm->returnToMenu(); }

void NewsView::drawNewsHeader()
{
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString("LATEST NEWS", 75, 12);

    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);
}

void NewsView::drawNewsContent()
{
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(UI::COL_TEXT_DIM);
    _tft->setFont(UI::Fonts::BODY_MAIN);

    int cy = UI::HEADER_H + (UI::CONTENT_H / 2);
    _tft->drawString("News Feed Coming Soon", UI::SCREEN_W / 2, cy - 20);

    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextColor(UI::COL_MUTED);
    _tft->drawString("Long press to return", UI::SCREEN_W / 2, cy + 20);
}
