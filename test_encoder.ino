#include <Arduino.h>
#include "src/input/EncoderInput.h"

int encoderValue = 0;

void onEncoderChanged(int value) {
    encoderValue = value;
    Serial.print("Encoder: ");
    Serial.println(encoderValue);
}

void onEncoderPressed() {
    encoderValue = 0;
    Serial.println("Reset to 0");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Encoder Test Starting...");
    
    // Initialize encoder
    encoder_init(-100, 100, 0);
    
    Serial.println("Encoder Test Ready");
    Serial.println("Rotate encoder to test, press button to reset");
}

void loop() {
    encoder_loop();
    delay(10);
}