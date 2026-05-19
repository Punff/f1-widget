#pragma once
#include <cstdint>

class LGFX;

class Footer {
public:
    explicit Footer(LGFX *tft);

    void draw();
    void drawText(const char *text, uint32_t color = 0x777777);

private:
    LGFX *_tft;
};
