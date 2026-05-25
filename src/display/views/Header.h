#pragma once
#include <cstdint>

class LGFX;
class TimeManager;

class Header
{
public:
    explicit Header(LGFX *tft);

    void draw(const char *title, const char *subtitle = nullptr, const char *prefix = nullptr);
    void tick(TimeManager *tm);
    void encoderPulse(int dir);
    void encoderPress();
    void encoderLongPress();
    void redrawEncoder();
    bool encoderActive() const;
    void markDirty();

private:
    LGFX *_tft;
    uint32_t _glowColor;
    unsigned long _glowMs;
    int _lastClockMin;

    void drawClock(TimeManager *tm);
    void drawEncoderDot();

    // Encoder indicator — bottom-right glowing dot
    static constexpr int DOT_X = 458;
    static constexpr int DOT_Y = 300;
    static constexpr int DOT_R = 5;
    static constexpr unsigned long COLOR_MS = 80;
    static constexpr unsigned long WHITE_DECAY_MS = 400;

    // Clock — top-right
    static constexpr int CLOCK_RX = 470;
    static constexpr int CLOCK_Y = 9;
    static constexpr int CLOCK_W = 80;
    static constexpr int CLOCK_H = 22;
};
