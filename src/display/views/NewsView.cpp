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
    drawFooter();
}

void NewsView::render() {}

void NewsView::onLongPress() { _dm->returnToMenu(); }

void NewsView::drawNewsHeader()
{
    _dm->header()->draw("LATEST NEWS");
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

void NewsView::drawFooter()
{
    _dm->footer()->draw();
    _dm->footer()->drawCenter("NEW \xc2\xb7 Coming soon", UI::COL_MUTED);
}
