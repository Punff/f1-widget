#pragma once
#include <cstdint>

// MenuItem enum must be in global namespace
enum class MenuItem
{
    DRIVER_STANDINGS = 0,
    CONSTRUCTOR_STANDINGS,
    CALENDAR,
    NEWS,
    SETTINGS,
    COUNT
};

namespace UI
{
    // Screen dimensions - 3.5" TN panel 480x320
    static constexpr int SCREEN_W = 480;
    static constexpr int SCREEN_H = 320;
    static constexpr int SAFE_W = SCREEN_W; // Full width, no side strips

    // Layout
    static constexpr int HEADER_H = 50;
    static constexpr int FOOTER_H = 40;
    static constexpr int FOOTER_Y = SCREEN_H - FOOTER_H;
    static constexpr int CONTENT_Y = HEADER_H;
    static constexpr int CONTENT_H = FOOTER_Y - HEADER_H;

    // F1 TN-Optimized Palette (High Contrast) — 24-bit RGB (0xRRGGBB)
    static constexpr uint32_t COL_BG = 0x081008;        // Dark slate
    static constexpr uint32_t COL_BG_SEL = 0x2A2A2A;    // Selected row (brighter for TN)
    static constexpr uint32_t COL_BG_ALT = 0x0D0D0D;    // Alternate row
    static constexpr uint32_t COL_TEXT = 0xFFFFFF;      // Pure white
    static constexpr uint32_t COL_TEXT_DIM = 0xAAAAAA;  // Dimmed text
    static constexpr uint32_t COL_MUTED = 0x787C78;     // Chassis grey
    static constexpr uint32_t COL_F1_RED = 0xE00000;    // Neon red
    static constexpr uint32_t COL_F1_YELLOW = 0xFFD700; // F1 yellow accent
    static constexpr uint32_t COL_F1_PURPLE = 0xAA00FF; // Fastest lap purple
    static constexpr uint32_t COL_ACCENT = 0xF0D400;    // Podium yellow
    static constexpr uint32_t COL_DIVIDER = 0x333333;   // Row separators

    static constexpr int FONT_TINY = 1;
    static constexpr int FONT_SMALL = 1;
    static constexpr int FONT_MEDIUM = 2;
    static constexpr int FONT_LARGE = 3;

    // Spacing
    static constexpr int PAD_X = 12;
    static constexpr int PAD_Y = 8;

    // Shared sprite max row height — all views use same sprite to prevent heap fragmentation
    static constexpr int MAX_ROW_H = 50;

    // Encoder visual feedback
    static constexpr int ENCODER_PULSE_MS = 30;
}
