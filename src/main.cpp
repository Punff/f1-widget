#include <Arduino.h>
#include "input/EncoderInput.h"
#include "LGFX_Config.h"

LGFX tft;

int encoderValue = 0;

void onEncoderChanged(int value)
{
    encoderValue = value;
    Serial.print("Encoder: ");
    Serial.println(encoderValue);
}

void onEncoderPressed()
{
    Serial.println("Single press");
}

void onEncoderLongPress()
{
    Serial.println("Long press");
}

void onEncoderDoublePress()
{
    Serial.println("Double press");
}

void setup()
{
    Serial.begin(115200);

    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_GREEN);
    tft.drawString("LGFX OK!", 80, 100);

    encoder_init(-100, 100, 0);
}

void loop()
{
    encoder_loop();
    delay(5);
}