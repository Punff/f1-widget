#include "EncoderWidget.h"
#include <math.h>

// Ensure the class name and arguments match the header exactly
EncoderWidget::EncoderWidget(LGFX *tft)
    : _tft(tft),
      _canvas(tft),
      _rotOffset(0),
      _pulse(0),
      _glowAlpha(0),
      _glowCol(0)
{
    // Constructor body can stay empty
}
void EncoderWidget::init()
{
    _canvas.setColorDepth(16);
    _canvas.createSprite(120, 220);
    _draw();
}

void EncoderWidget::notify(EncoderWidgetState state)
{
    // 1. Set Glow Color based on input type
    if (state == EncoderWidgetState::PRESS)
        _glowCol = TFT_RED;
    else if (state == EncoderWidgetState::LONG_PRESS)
        _glowCol = TFT_ORANGE;
    else
        _glowCol = TFT_CYAN; // Turn colors

    _glowAlpha = 255; // Trigger the flash

    // 2. Set Rotation
    if (state == EncoderWidgetState::TURN_UP)
        _rotOffset = 15.0f;
    if (state == EncoderWidgetState::TURN_DOWN)
        _rotOffset = -15.0f;
    _draw();
}

void EncoderWidget::loop()
{
    bool update = false;

    // Smooth decay for rotation
    if (abs(_rotOffset) > 0.1f)
    {
        _rotOffset *= 0.6f;
        update = true;
    }

    // Pulsating sine wave (Breathing)
    _pulse += 0.12f;
    if (_pulse > 6.28f)
        _pulse = 0;

    // Fade out the glow
    if (_glowAlpha > 0)
    {
        _glowAlpha -= 15;
        update = true;
    }

    // Redraw every ~40ms for the pulse even if no input
    if (update || (millis() % 40 < 20))
        _draw();
}

void EncoderWidget::_draw()
{
    _canvas.fillSprite(TFT_BLACK);
    int LCX = 110;
    int LCY = 110;
    float s = sin(_pulse);

    // 1. ROTATING GREY TEETH
    for (int i = -3; i <= 3; i++)
    {
        float angle = 180.0f + (i * 28.0f) + _rotOffset;
        float rad = (angle * M_PI) / 180.0f;
        int x1 = LCX + (int)(cos(rad) * RADIUS);
        int y1 = LCY + (int)(sin(rad) * RADIUS);
        int x2 = LCX + (int)(cos(rad) * (RADIUS - 12));
        int y2 = LCY + (int)(sin(rad) * (RADIUS - 12));
        _canvas.drawLine(x1, y1, x2, y2, 0x4208);
    }

    // 2. THE DYNAMIC MOUNTING PLATE
    int plateThickness = 6 + (int)(s * 2);
    uint16_t plateCol = 0x4208; // Default Grey

    if (_glowAlpha > 0)
    {
        // Manual Blend: Blend _glowCol into 0x4208 based on _glowAlpha
        uint16_t bg = 0x4208;
        uint16_t fg = _glowCol;

        uint8_t r = (((fg >> 11) & 0x1F) * _glowAlpha + ((bg >> 11) & 0x1F) * (255 - _glowAlpha)) >> 8;
        uint8_t g = (((fg >> 5) & 0x3F) * _glowAlpha + ((bg >> 5) & 0x3F) * (255 - _glowAlpha)) >> 8;
        uint8_t b = ((fg & 0x1F) * _glowAlpha + (bg & 0x1F) * (255 - _glowAlpha)) >> 8;

        plateCol = (r << 11) | (g << 5) | b;
    }

    // Draw the main plate arc
    _canvas.drawArc(LCX, LCY, RADIUS + 2, RADIUS - plateThickness, 165, 195, plateCol);

    // 3. THE GLOW EFFECT (Subtle outer ring)
    if (_glowAlpha > 0)
    {
        // Just dim the glow color for the outer ring
        uint8_t r = ((_glowCol >> 11) & 0x1F) * _glowAlpha / 255;
        uint8_t g = ((_glowCol >> 5) & 0x3F) * _glowAlpha / 255;
        uint8_t b = (_glowCol & 0x1F) * _glowAlpha / 255;
        uint16_t outerGlow = (r << 11 | g << 5 | b);
        _canvas.drawArc(LCX, LCY, RADIUS + 4, RADIUS + 3, 160, 200, outerGlow);
    }

    // 4. DATA PORT (Small white indicator at exactly 9 o'clock)
    _canvas.drawArc(LCX, LCY, RADIUS + 2, RADIUS - 2, 179, 181, TFT_WHITE);

    // 5. THE CONNECTION RAIL (Matches the MenuView line)
    int tx = LCX - RADIUS;
    _canvas.fillRect(tx - 20, LCY - 1, 20, 2, 0x4208);

    _canvas.pushSprite(CX - 110, CY - 110);
}