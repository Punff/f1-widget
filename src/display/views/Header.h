#pragma once
#include <cstdint>
#include "UI.h"

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
    int _lastClockHour;
    bool _clockStale;

    void drawClock(TimeManager *tm);
    void drawEncoderDot();

    // Header grid constants
    static constexpr int H_PAD = 10;
    static constexpr int H_CENTER_Y = UI::HEADER_H / 2;
    static constexpr int H_CLOCK_X = UI::SCREEN_W - 5;
    static constexpr int H_DOT_X = UI::SCREEN_W - 55;
    static constexpr int H_DOT_Y = H_CENTER_Y;
    static constexpr int H_DOT_R = 5;

    // Clock — top-right
    static constexpr int CLOCK_W = 80;
    static constexpr int CLOCK_H = 22;

    static constexpr unsigned long COLOR_MS = 80;
    static constexpr unsigned long WHITE_DECAY_MS = 400;
};
