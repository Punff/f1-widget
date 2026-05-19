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

    // F1 TN-Optimized Palette (High Contrast)
    static constexpr uint32_t COL_BG = 0x000000;        // Pure black
    static constexpr uint32_t COL_BG_SEL = 0x1A1A1A;    // Dark grey selected row
    static constexpr uint32_t COL_BG_ALT = 0x0D0D0D;    // Alternate row
    static constexpr uint32_t COL_TEXT = 0xFFFFFF;      // Pure white
    static constexpr uint32_t COL_TEXT_DIM = 0xAAAAAA;  // Dimmed text
    static constexpr uint32_t COL_MUTED = 0x777777;     // Subtle grey
    static constexpr uint32_t COL_F1_RED = 0xFF0000;    // Pure F1 red
    static constexpr uint32_t COL_F1_YELLOW = 0xFFD700; // F1 yellow accent
    static constexpr uint32_t COL_ACCENT = 0x00FF00;    // Green for gains
    static constexpr uint32_t COL_DIVIDER = 0x333333;   // Row separators

    static constexpr int FONT_TINY = 1;
    static constexpr int FONT_SMALL = 1;
    static constexpr int FONT_MEDIUM = 2;
    static constexpr int FONT_LARGE = 3;

    // Spacing
    static constexpr int PAD_X = 12;
    static constexpr int PAD_Y = 8;

    // Animation
    static constexpr int SCROLL_DURATION = 200; // ms for smooth scroll
    static constexpr float DIM_RATE = 0.22f;    // Brightness drop per row
    static constexpr float MIN_BRIGHT = 0.15f;  // Never fully black

    // Shared sprite max row height — all views use same size to prevent heap fragmentation
    static constexpr int MAX_ROW_H = 50;

    // Encoder visual feedback (subtle)
    static constexpr int ENCODER_PULSE_MS = 30;           // Shorter, subtle flash
    static constexpr uint32_t ENCODER_FLASH_COL = 0x6800; // Dark red, subtle
}
