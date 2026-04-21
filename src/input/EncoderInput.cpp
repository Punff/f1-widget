#include "EncoderInput.h"

AiEsp32RotaryEncoder encoder(
    ENCODER_CLK,
    ENCODER_DT,
    ENCODER_SW,
    -1,
    ENCODER_STEPS);

static unsigned long buttonDownTime = 0;
static unsigned long buttonReleaseTime = 0;
static unsigned long lastButtonStateChange = 0;
static bool buttonHandled = false;
static bool waitingDoublePress = false;
static bool doublePressHandled = false;
static bool lastButtonState = false;

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
}

void encoder_loop()
{
    if (encoder.encoderChanged())
    {
        onEncoderChanged(encoder.readEncoder());
    }

    bool buttonDown = encoder.isEncoderButtonDown();
    unsigned long now = millis();

    bool stableButtonDown = buttonDown;
    if (buttonDown != lastButtonState)
    {
        if ((now - lastButtonStateChange) < DEBOUNCE_MS)
        {
            stableButtonDown = lastButtonState;
        }
        else
        {
            lastButtonStateChange = now;
            lastButtonState = buttonDown;
            stableButtonDown = buttonDown;
        }
    }

    if (stableButtonDown && !buttonHandled)
    {
        if (waitingDoublePress)
        {
            onEncoderDoublePress();
            waitingDoublePress = false;
            doublePressHandled = true;
        }
        buttonDownTime = now;
        buttonReleaseTime = 0;
        buttonHandled = true;
    }
    else if (!stableButtonDown && buttonHandled)
    {
        unsigned long duration = now - buttonDownTime;
        buttonHandled = false;

        if (duration >= LONG_PRESS_MS)
        {
            onEncoderLongPress();
        }
        else if (!doublePressHandled)
        {
            waitingDoublePress = true;
            buttonReleaseTime = now;
        }
        else
        {
            doublePressHandled = false;
        }
    }

    if (waitingDoublePress && buttonReleaseTime > 0 && (now - buttonReleaseTime) >= DOUBLE_PRESS_WINDOW_MS)
    {
        onEncoderPressed();
        waitingDoublePress = false;
        buttonReleaseTime = 0;
    }
}