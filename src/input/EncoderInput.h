#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "config.h"

#define ENCODER_STEPS 4

extern AiEsp32RotaryEncoder encoder;

void encoder_init(int minVal, int maxVal, int startVal);
void encoder_loop();

// Callbacks — implement these in your main logic
void onEncoderChanged(int value);
void onEncoderPressed();
void onEncoderLongPress();
void onEncoderDoublePress();

#endif