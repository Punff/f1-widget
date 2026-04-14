#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "../config.h"

class DisplayManager {
public:
    DisplayManager();
    void init();
    void clear();
    void showText(int x, int y, const char* text, uint16_t color = TFT_WHITE, uint8_t size = 2);
    void showNumber(int x, int y, int num, uint16_t color = TFT_YELLOW, uint8_t size = 3);
    TFT_eSPI* getTft();
};

#endif
