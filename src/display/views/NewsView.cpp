#include "NewsView.h"
#include "../DisplayManager.h"
#include "../../../include/LGFX_Config.h"

NewsView::NewsView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _articleIndex(0) {}

void NewsView::onEnter() {}
void NewsView::onExit() {}
void NewsView::render() {}
void NewsView::onTurnRight() {}
void NewsView::onTurnLeft() {}
void NewsView::onPress() {}
void NewsView::onLongPress() {}
void NewsView::onDoublePress() {}