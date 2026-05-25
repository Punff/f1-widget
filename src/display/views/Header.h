#pragma once
#include <cstdint>

class LGFX;

class Header
{
public:
    explicit Header(LGFX *tft);

    void draw(const char *title, const char *subtitle = nullptr);
    void encoderPulse(int dir);
    void encoderPress();
    void encoderLongPress();
    void redrawEncoder();
    bool encoderActive() const;

private:
    LGFX *_tft;
    uint32_t _glowColor;
    d unsigned long _glowMs;

    void drawEncoderArc();

    // Quarter-circle in top-right corner, bulging toward (480,0).
    // Center at (456,24), R=24, arc 0°→90° connects right edge (480,24) to top edge (456,0).
    static constexpr int ARC_CX = 480;
    static constexpr int ARC_CY = 0;
    static constexpr int ARC_RMIN = 23;
    static constexpr int ARC_RMAX = 24;
    static constexpr int ARC_START = 90;
    static constexpr int ARC_END = 270;
    static constexpr unsigned long COLOR_MS = 50;        // Brief color burst
    static constexpr unsigned long WHITE_DECAY_MS = 200; // Short cooldown to white
};
