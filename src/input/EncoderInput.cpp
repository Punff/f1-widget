#include "EncoderInput.h"

AiEsp32RotaryEncoder encoder(
    ENCODER_CLK,
    ENCODER_DT,
    ENCODER_SW,
    -1,
    ENCODER_STEPS);

enum BtnState
{
    BTN_IDLE,
    BTN_PRESSED,
    BTN_WAIT_DOUBLE
};

static BtnState buttonState = BTN_IDLE;
static unsigned long buttonDownTime = 0;
static unsigned long buttonUpTime = 0;
static int lastEncValue = 0;

void IRAM_ATTR readEncoderISR()
{
    encoder.readEncoder_ISR();
}

void encoder_init(int minVal, int maxVal, int startVal)
{
    encoder.begin();
    encoder.setup(readEncoderISR);
    encoder.setBoundaries(minVal, maxVal, false);
    encoder.setEncoderValue(startVal);
    encoder.setAcceleration(0);
    lastEncValue = startVal;
}

void encoder_loop()
{
    // ── Rotation ────────────────────────────────────────────
    if (encoder.encoderChanged())
    {
        int val = encoder.readEncoder();
        if (val > lastEncValue)
        {
            onEncoderTurnRight();
        }
        else if (val < lastEncValue)
        {
            onEncoderTurnLeft();
        }
        lastEncValue = val;
    }

    // ── Button state machine ─────────────────────────────────
    bool buttonDown = encoder.isEncoderButtonDown();
    unsigned long now = millis();

    switch (buttonState)
    {
    case BTN_IDLE:
        if (buttonDown)
        {
            buttonDownTime = now;
            buttonState = BTN_PRESSED;
        }
        break;

    case BTN_PRESSED:
        if (!buttonDown)
        {
            unsigned long held = now - buttonDownTime;
            buttonUpTime = now;
            if (held >= LONG_PRESS_MS)
            {
                onEncoderLongPress();
                buttonState = BTN_IDLE;
            }
            else
            {
                buttonState = BTN_WAIT_DOUBLE;
            }
        }
        else if ((now - buttonDownTime) >= LONG_PRESS_MS)
        {
            onEncoderLongPress();
            buttonState = BTN_IDLE;
        }
        break;

    case BTN_WAIT_DOUBLE:
        if (buttonDown)
        {
            onEncoderDoublePress();
            buttonState = BTN_IDLE;
        }
        else if ((now - buttonUpTime) >= DOUBLE_PRESS_WINDOW_MS)
        {
            onEncoderPressed();
            buttonState = BTN_IDLE;
        }
        break;
    }
}