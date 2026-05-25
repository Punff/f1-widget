#include "Footer.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"

static constexpr int CENTER_X = 240;
static constexpr int CENTER_W = 240;
static constexpr int CENTER_H = 16;
static constexpr int WIFI_X = 435;
static constexpr int WIFI_R = 4;

Footer::Footer(LGFX *tft) : _tft(tft), _wifiOn(false) {
    _tooltip[0] = '\0';
}

void Footer::setTooltip(const char *t) {
    strlcpy(_tooltip, t, sizeof(_tooltip));
}

void Footer::setWifiConnected(bool on) {
    _wifiOn = on;
}

void Footer::draw() {
    int cy = UI::FOOTER_Y + UI::FOOTER_H / 2;

    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);
    _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, 2, UI::COL_F1_RED);

    uint32_t wifiCol = _wifiOn ? UI::COL_ACCENT : UI::COL_MUTED;
    _tft->fillCircle(WIFI_X, cy, WIFI_R, wifiCol);
}

void Footer::drawCenter(const char *text, uint32_t color) {
    if (!text || !text[0]) return;
    int cy = UI::FOOTER_Y + UI::FOOTER_H / 2;
    _tft->fillRect(CENTER_X - CENTER_W / 2, cy - CENTER_H / 2, CENTER_W, CENTER_H, UI::COL_BG);
    _tft->setTextDatum(middle_center);
    _tft->setTextColor(color);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(text, CENTER_X, cy);
}

void Footer::redrawWifi() {
    int cy = UI::FOOTER_Y + UI::FOOTER_H / 2;
    uint32_t wifiCol = _wifiOn ? UI::COL_ACCENT : UI::COL_MUTED;
    _tft->fillCircle(WIFI_X, cy, WIFI_R, wifiCol);
}
