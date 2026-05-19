#pragma once
#include <cstdint>

class LGFX;

class Header {
public:
    explicit Header(LGFX *tft);

    void draw(const char *title, const char *subtitle = nullptr);
    void encoderPulse(int dir);
    void encoderPress();
    void encoderLongPress();
    void redrawEncoder();

private:
    LGFX *_tft;
    uint32_t _glowColor;
    unsigned long _glowMs;

    void drawEncoderArc();

    // Quarter-circle in top-right corner, mirrored so it bulges toward corner.
    static constexpr int ARC_CX = 480;
    static constexpr int ARC_CY = 24;
    static constexpr int ARC_RMIN = 23;
    static constexpr int ARC_RMAX = 24;
    static constexpr int ARC_START = 180;
    static constexpr int ARC_END = 270;
    static constexpr unsigned long FLASH_MS = 50;
    static constexpr unsigned long GLOW_FADE_MS = 350;
};
