#include <Arduino.h>
#include "input/EncoderInput.h"
#include "display/DisplayManager.h"
#include <TFT_eSPI.h>

DisplayManager display;
int encoderValue = 0;
unsigned long lastUpdate = 0;

void onEncoderChanged(int value) {
    encoderValue = value;
    Serial.print("Encoder: ");
    Serial.println(encoderValue);
    
    // Update display every 100ms to avoid flicker
    unsigned long now = millis();
    if (now - lastUpdate > 100) {
        lastUpdate = now;
        
        // Update display
        display.clear();
        display.showText(30, 30, "F1 Widget Test", TFT_CYAN, 3);
        display.showText(30, 80, "Encoder Value:", TFT_WHITE, 2);
        display.showNumber(30, 120, encoderValue, TFT_YELLOW, 4);
        display.showText(30, 180, "Press to reset", TFT_GREEN, 2);
        
        // Show encoder direction indicator
        TFT_eSPI* tft = display.getTft();
        if (value > 0) {
            tft->fillTriangle(250, 140, 270, 120, 270, 160, TFT_GREEN);
            tft->setTextColor(TFT_GREEN);
            tft->setTextSize(2);
            tft->drawString("CW", 280, 135);
        } else if (value < 0) {
            tft->fillTriangle(250, 140, 230, 120, 230, 160, TFT_RED);
            tft->setTextColor(TFT_RED);
            tft->setTextSize(2);
            tft->drawString("CCW", 200, 135);
        }
    }
}

void onEncoderPressed() {
    encoderValue = 0;
    Serial.println("Reset to 0");
    
    // Update display
    display.clear();
    display.showText(30, 30, "F1 Widget Test", TFT_CYAN, 3);
    display.showText(30, 80, "Encoder Value:", TFT_WHITE, 2);
    display.showNumber(30, 120, encoderValue, TFT_YELLOW, 4);
    display.showText(30, 180, "Press to reset", TFT_GREEN, 2);
    display.showText(30, 220, "RESET!", TFT_RED, 2);
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Wait for serial to stabilize
    Serial.println("F1 Widget Display+Encoder Test Starting...");
    
    // Initialize display
    Serial.println("Initializing display...");
    display.init();
    Serial.println("Display init called");
    
    // Try to show something simple first
    Serial.println("Attempting to draw test pattern...");
    display.clear();
    display.showText(30, 30, "F1 Test", TFT_WHITE, 2);
    display.showNumber(30, 80, 123, TFT_YELLOW, 3);
    
    // Initialize encoder
    Serial.println("Initializing encoder...");
    encoder_init(-100, 100, 0);
    
    Serial.println("Test Ready - rotate encoder or press button");
    Serial.println("If you see this but no display, check TFT pins");
}

void loop() {
    encoder_loop();
    delay(10);
}