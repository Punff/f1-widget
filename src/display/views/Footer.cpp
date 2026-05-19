#include "Footer.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"

Footer::Footer(LGFX *tft) : _tft(tft) {}

void Footer::draw() {
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);
    _tft->drawFastHLine(0, UI::FOOTER_Y, UI::SCREEN_W, UI::COL_F1_RED);
}

void Footer::drawText(const char *text, uint32_t color) {
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(color);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(text, UI::SCREEN_W / 2, UI::FOOTER_Y + UI::FOOTER_H / 2);
}
