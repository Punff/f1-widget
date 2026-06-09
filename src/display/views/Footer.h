#pragma once
#include <cstdint>
#include "UI.h"

class LGFX;

class Footer {
public:
    explicit Footer(LGFX *tft);

    void setWifiConnected(bool on);
    void draw();
    void drawCenter(const char *text, uint32_t color = UI::COL_MUTED);
    void redrawWifi();
    void markDirty() { _dirty = true; }

private:
    void animateCenter(const char *newText, uint32_t color);
    LGFX *_tft;
    char _lastCenterText[48];
    uint32_t _lastCenterColor;
    bool _wifiOn;
    bool _dirty;
};
