#pragma once
#include <cstdint>

class LGFX;

class Footer {
public:
    explicit Footer(LGFX *tft);

    void setTooltip(const char *t);
    void setWifiConnected(bool on);
    void draw();
    void drawCenter(const char *text, uint32_t color = 0x777777);
    void redrawWifi();
    void markDirty() { _dirty = true; }

private:
    LGFX *_tft;
    char _tooltip[32];
    char _lastCenterText[48];
    uint32_t _lastCenterColor;
    bool _wifiOn;
    bool _dirty;
};
