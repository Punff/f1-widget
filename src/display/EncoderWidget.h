#pragma once
#include "../../include/LGFX_Config.h"

enum class EncoderWidgetState
{
    IDLE,
    TURN_UP,
    TURN_DOWN,
    PRESS,
    LONG_PRESS,
    DOUBLE_PRESS
};

class EncoderWidget
{
public:
    EncoderWidget(LGFX *tft);
    void init();
    void notify(EncoderWidgetState state);
    void loop();

    static constexpr int CX = 495;
    static constexpr int CY = 160;
    static constexpr int RADIUS = 82;
    static constexpr int TRI_TIP_X = CX - RADIUS - 22;
    
private:
    LGFX *_tft;
    LGFX_Sprite _canvas;
    float _rotOffset = 0;
    float _pulse = 0;
    int _glowAlpha = 0;
    uint32_t _glowCol = 0;

    void _draw();
};