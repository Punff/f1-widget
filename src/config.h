#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Display - LovyanGFX panel config
#define TFT_BCKL 27

// Encoder pins
#define ENCODER_CLK 22
#define ENCODER_DT 21
#define ENCODER_SW 35

// Input timing
#define LONG_PRESS_MS 600
#define DOUBLE_PRESS_WINDOW_MS 200
#define DEBOUNCE_MS 100

// WiFi
#define WIFI_AP_NAME "F1Widget"

// API
#define OPENF1_API_BASE_URL "https://api.openf1.org/v1"
#define API_UPDATE_INTERVAL 600000

// Display dimensions (LovyanGFX Panel_ST7796)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

#endif