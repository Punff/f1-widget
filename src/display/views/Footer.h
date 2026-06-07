#pragma once
#include <cstdint>

class LGFX;

class Footer {
public:
    explicit Footer(LGFX *tft);

    void setWifiConnected(bool on);
    void draw();
    void drawCenter(const char *text, uint32_t color = 0x777777);
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
