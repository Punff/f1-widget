#include "EncoderInput.h"

AiEsp32RotaryEncoder encoder(
    ENCODER_CLK,
    ENCODER_DT,
    ENCODER_SW,
    -1, // VCC pin — set to -1, wire VCC directly to 3.3V
    ENCODER_STEPS);

void IRAM_ATTR readEncoderISR()
{
    encoder.readEncoder_ISR();
}

void encoder_init(int minVal, int maxVal, int startVal)
{
    encoder.begin();
    encoder.setup(readEncoderISR);
    encoder.setBoundaries(minVal, maxVal, false); // false = no wrap
    encoder.setEncoderValue(startVal);
    encoder.setAcceleration(0); // disable acceleration
}

void encoder_loop()
{
    if (encoder.encoderChanged())
    {
        onEncoderChanged(encoder.readEncoder());
    }

    if (encoder.isEncoderButtonClicked())
    {
        onEncoderPressed();
    }
}