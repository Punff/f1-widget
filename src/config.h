#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── Display & Hardware ───────────────────────────────────────────────
// Encoder pins
#define ENCODER_CLK 22
#define ENCODER_DT 21
#define ENCODER_SW 35

// Input timing
#define LONG_PRESS_MS 600
#define DOUBLE_PRESS_WINDOW_MS 200

// Backlight Pin
#define TFT_BCKL 27

// ── App Settings ─────────────────────────────────────────────────────
#define APP_VERSION "v1.1"

// ── Network & APIs ───────────────────────────────────────────────────
// WiFi Config
#define WIFI_AP_NAME "F1Widget"

// API Endpoints
#define OPENF1_API_BASE "https://api.openf1.org/v1"
#define JOLPICA_API_BASE "https://api.jolpi.ca/ergast/f1"
#define RSS_NEWS_URL "https://www.motorsport.com/rss/f1/news/"

// Auto-Sync Settings
#define API_UPDATE_INTERVAL_MS 3600000 // Background update interval in milliseconds (default: 1 hour)

#endif