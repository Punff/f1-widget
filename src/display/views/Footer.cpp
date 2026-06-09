#include "Footer.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"
#include <WiFi.h>

static constexpr int CENTER_X = 240;
static constexpr int CENTER_W = 240;
static constexpr int CENTER_H = 16;
static constexpr int WIFI_X = 460;
static constexpr int WIFI_Y = UI::FOOTER_Y + 12;

Footer::Footer(LGFX *tft) : _tft(tft), _wifiOn(false), _dirty(true) {
    _lastCenterText[0] = '\0';
    _lastCenterColor = 0;
}

void Footer::setWifiConnected(bool on) {
    if (_wifiOn != on) {
        _wifiOn = on;
        _dirty = true;
    }
}

void Footer::draw() {
    _lastCenterText[0] = '\0'; // Force center text redraw
    if (!_dirty) return;
    
    _tft->waitDMA();
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, 2, UI::COL_ACCENT);
    redrawWifi();
    _dirty = false;
}

void Footer::drawCenter(const char *text, uint32_t color) {
    if (!text || !text[0]) return;
    
    bool textChanged = strcmp(text, _lastCenterText) != 0;

    // Skip if same as last draw and not dirty
    if (!_dirty && !textChanged && color == _lastCenterColor) {
        return;
    }

    if (textChanged && _lastCenterText[0]) {
        animateCenter(text, color);
    } else {
        int cy = UI::FOOTER_Y + UI::FOOTER_H / 2;
        _tft->waitDMA();
        _tft->fillRect(CENTER_X - CENTER_W / 2, cy - CENTER_H / 2, CENTER_W, CENTER_H, UI::COL_BG);
        _tft->setTextDatum(middle_center);
        _tft->setTextColor(color);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString(text, CENTER_X, cy);
    }

    strlcpy(_lastCenterText, text, sizeof(_lastCenterText));
    _lastCenterColor = color;
}

void Footer::animateCenter(const char *newText, uint32_t color) {
    int cy = UI::FOOTER_Y + UI::FOOTER_H / 2;
    int cx = UI::SCREEN_W / 2;

    // 3 frames, 3px per step = 24ms total
    for (int step = 2; step >= 0; step--) {
        _tft->waitDMA();
        // Clear below accent line (FOOTER_Y+2) to leave footer accent bar intact
        _tft->fillRect(cx - CENTER_W / 2, UI::FOOTER_Y + 2, CENTER_W, UI::FOOTER_H - 2, UI::COL_BG);
        _tft->setTextDatum(middle_center);
        _tft->setFont(UI::Fonts::LABEL_SMALL);

        _tft->setTextColor(UI::COL_MUTED);
        _tft->drawString(_lastCenterText, cx, cy - (2 - step) * 3);

        _tft->setTextColor(color);
        _tft->drawString(newText, cx, cy + (step + 1) * 3);

        delay(8);
    }
    // Final: ensure clean draw at resting position
    _tft->waitDMA();
    _tft->fillRect(cx - CENTER_W / 2, UI::FOOTER_Y + 2, CENTER_W, UI::FOOTER_H - 2, UI::COL_BG);
    _tft->setTextDatum(middle_center);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->setTextColor(color);
    _tft->drawString(newText, cx, cy);
}

void Footer::redrawWifi() {
    // Clear WiFi area
    _tft->fillRect(WIFI_X, WIFI_Y, 24, 20, UI::COL_BG);

    if (!_wifiOn) {
        _tft->setTextColor(UI::COL_F1_RED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString("!", WIFI_X + 10, WIFI_Y + 10);
        return;
    }

    int32_t rssi = WiFi.RSSI();
    int bars = 0;
    if (rssi > -60) bars = 3;
    else if (rssi > -75) bars = 2;
    else if (rssi > -90) bars = 1;

    for (int i = 0; i < 3; i++) {
        uint32_t col = (i < bars) ? UI::COL_ACCENT : UI::COL_MUTED;
        int h = (i + 1) * 6;
        _tft->fillRect(WIFI_X + (i * 6), WIFI_Y + (18 - h), 4, h, col);
    }
}
