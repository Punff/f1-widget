#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// TFT_eSPI pins (ESP32-3248S035 - ST7796)
// Updated to match Setup27_RPi_ST7796_ESP32.h configuration
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
#define TFT_BCKL 27

// Encoder pins (your wiring with VCC)
#define ENCODER_CLK 22 // CLK (A)
#define ENCODER_DT 21  // DT (B)
#define ENCODER_SW 35  // Button (now with VCC)

// WiFi
#define WIFI_AP_NAME "F1Widget"

// API
#define OPENF1_API_BASE_URL "https://api.openf1.org/v1"
#define API_UPDATE_INTERVAL 600000

// Display
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#endif