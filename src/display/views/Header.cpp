#include "Header.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"

Header::Header(LGFX *tft)
    : _tft(tft), _glowColor(0), _glowMs(0) {}

void Header::draw(const char *title, const char *subtitle) {
    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    _tft->setTextDatum(top_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString("F1", 10, 8);

    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(title, 75, 12);

    if (subtitle) {
        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString(subtitle, 75, 32);
    }

    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

    drawEncoderArc();
}

void Header::encoderPulse(int dir) {
    _glowColor = (dir > 0) ? UI::COL_F1_RED : UI::COL_TEXT;
    _glowMs = millis();
    redrawEncoder();
}

void Header::encoderPress() {
    _glowColor = UI::COL_F1_YELLOW;
    _glowMs = millis();
    redrawEncoder();
}

void Header::encoderLongPress() {
    _glowColor = UI::COL_F1_RED;
    _glowMs = millis();
    redrawEncoder();
}

void Header::redrawEncoder() {
    drawEncoderArc();
}

bool Header::encoderActive() const {
    return millis() - _glowMs < WHITE_DECAY_MS;
}

void Header::drawEncoderArc() {
    unsigned long now = millis();
    unsigned long elapsed = now - _glowMs;

    if (elapsed < COLOR_MS) {
        // Phase 1: thick color burst — tactile impact in red/white/yellow
        _tft->fillArc(ARC_CX, ARC_CY, ARC_RMIN - 1, ARC_RMAX + 1, ARC_START, ARC_END, _glowColor);
    } else if (elapsed < WHITE_DECAY_MS) {
        // Phase 2: settles to thin white — premium glow decay
        _tft->fillArc(ARC_CX, ARC_CY, ARC_RMIN, ARC_RMAX, ARC_START, ARC_END, UI::COL_TEXT);
    } else {
        // Idle: thin white arc
        _tft->fillArc(ARC_CX, ARC_CY, ARC_RMIN, ARC_RMAX, ARC_START, ARC_END, UI::COL_TEXT);
    }
}
