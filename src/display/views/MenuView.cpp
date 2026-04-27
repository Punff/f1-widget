#include "MenuView.h"
#include "../DisplayManager.h"
#include "../EncoderWidget.h"
#include <math.h>

#ifndef DEG2RAD
#define DEG2RAD(a) ((a) * M_PI / 180.0f)
#endif

// Official F1 Palette Constants
static constexpr uint32_t COL_NEAR = 0xFFFFFF; // Active Text
static constexpr uint32_t COL_FAR = 0x4208;    // Dimmed Text (Graphite)
static constexpr uint32_t COL_LOCKIN = 0xF800; // Ferrari Red

MenuView::MenuView(LGFX *tft, DisplayManager *dm)
    : _tft(tft), _dm(dm), _faceAngle(180.0f), _activeIndex(0), _isMoving(false) {}

void MenuView::onEnter()
{
    _tft->fillScreen(0x000000);
    _faceAngle = _targetAngle(_activeIndex);
    _isMoving = false;
    _drawItems(_faceAngle);
    _drawLockIn(_activeIndex);
}

void MenuView::onExit() { _tft->fillScreen(0x000000); }

void MenuView::onTurnRight()
{
    _activeIndex = (_activeIndex + 1) % ITEM_COUNT;
    _isMoving = true;
    _eraseLockIn();
}

void MenuView::onTurnLeft()
{
    _activeIndex = (_activeIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
    _isMoving = true;
    _eraseLockIn();
}

void MenuView::tick()
{
    static unsigned long lastDraw = 0;
    if (millis() - lastDraw < 16)
        return; // cap at ~60fps
    lastDraw = millis();

    float target = _targetAngle(_activeIndex);
    float diff = target - _faceAngle;
    while (diff > 180.0f)
        diff -= 360.0f;
    while (diff < -180.0f)
        diff += 360.0f;

    if (fabsf(diff) > 1.0f)
    { // tighter threshold
        _faceAngle += diff * 0.25f;
        _erasePrev();
        _drawItems(_faceAngle);
        _isMoving = true;
    }
    else if (_isMoving)
    {
        _faceAngle = target;
        _erasePrev();
        _drawItems(_faceAngle);
        _drawLockIn(_activeIndex);
        _isMoving = false;
    }
}

// ── Helper Methods (Fixes Linker Errors) ──────────────────────────────────

void MenuView::_drawLockIn(int index)
{
    float target = _targetAngle(index);
    if (fabsf(target - _faceAngle) > 2.0f)
        return;

    int tx, ty;
    _itemPos(index, _faceAngle, tx, ty);

    const char *label = _label(index);
    int tw = _tft->textWidth(label);

    int x_start = tx - tw;
    int x_end = tx;
    int uy = ty + 14;

    // 1. THE RED UNDERLINE
    _tft->fillRect(x_start, uy, tw, 3, COL_LOCKIN);

    // 2. THE RAIL TRACE
    int anchorX = EncoderWidget::TRI_TIP_X;
    int anchorY = EncoderWidget::CY;

    _tft->drawFastHLine(x_end + 2, uy + 1, 10, COL_FAR);
    int dropX = x_end + 12;
    int dropStart = (uy + 1 < anchorY) ? uy + 1 : anchorY;
    int dropLen = abs((uy + 1) - anchorY);
    _tft->drawFastVLine(dropX, dropStart, dropLen, COL_FAR);
    _tft->drawFastHLine(dropX, anchorY, anchorX - dropX, COL_FAR);

    _prevLockIn.set(x_start, (uy < anchorY ? uy : anchorY), (anchorX - x_start) + 2, dropLen + 6);
}

void MenuView::_erasePrev()
{
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        if (!_prevRects[i].valid)
            continue;
        _tft->fillRect(_prevRects[i].x, _prevRects[i].y, _prevRects[i].w, _prevRects[i].h, 0x000000);
        _prevRects[i].valid = false;
    }
}

void MenuView::_drawItems(float faceAngle)
{
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        float angle = faceAngle + (i * ITEM_STEP);
        float rad = DEG2RAD(angle);

        float deg = fmodf(angle, 360.0f);
        if (deg < 0)
            deg += 360.0f;
        float dist = fabsf(deg - 180.0f);
        if (dist > 180.0f)
            dist = 360.0f - dist;

        if (dist > 80.0f)
            continue;

        int x = (int)(EncoderWidget::CX + cosf(rad) * TEXT_RADIUS);
        int y = (int)(EncoderWidget::CY + sinf(rad) * TEXT_RADIUS);

        uint32_t col = (i == _activeIndex) ? COL_NEAR : _lerpCol(COL_NEAR, COL_FAR, (dist / 80.0f));

        _tft->setTextColor(col, 0x000000);
        _tft->setTextDatum(textdatum_t::middle_right);
        _tft->setTextSize((i == _activeIndex) ? 2 : 1);

        _tft->drawString(_label(i), x, y);

        int tw = _tft->textWidth(_label(i)) + 10;
        int th = _tft->fontHeight() + 8;
        _prevRects[i].set(x - tw, y - (th / 2), tw, th);
    }
}

void MenuView::_eraseLockIn()
{
    if (!_prevLockIn.valid)
        return;
    _tft->fillRect(_prevLockIn.x, _prevLockIn.y, _prevLockIn.w, _prevLockIn.h, 0x000000);
    _prevLockIn.valid = false;
}

uint32_t MenuView::_lerpCol(uint32_t a, uint32_t b, float t) const
{
    if (t <= 0.0f)
        return a;
    if (t >= 1.0f)
        return b;
    uint8_t r1 = (a >> 16) & 0xFF, g1 = (a >> 8) & 0xFF, b1 = a & 0xFF;
    uint8_t r2 = (b >> 16) & 0xFF, g2 = (b >> 8) & 0xFF, b2 = b & 0xFF;
    uint8_t r = r1 + (r2 - r1) * t;
    uint8_t g = g1 + (g2 - g1) * t;
    uint8_t bv = b1 + (b2 - b1) * t;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bv;
}

const char *MenuView::_label(int index) const
{
    static const char *labels[] = {
        "DRIVER STANDINGS",
        "CONSTRUCTOR STANDINGS",
        "NEWS",
        "DRIVERS",
        "CIRCUITS",
        "CALENDAR",
        "SETTINGS"};
    return labels[index];
}

float MenuView::_targetAngle(int index) const { return 180.0f - (index * ITEM_STEP); }

void MenuView::_itemPos(int i, float faceAngle, int &x, int &y) const
{
    float rad = DEG2RAD(faceAngle + (i * ITEM_STEP));
    x = (int)(EncoderWidget::CX + cosf(rad) * TEXT_RADIUS);
    y = (int)(EncoderWidget::CY + sinf(rad) * TEXT_RADIUS);
}

void MenuView::render()
{
    _drawItems(_faceAngle);
    if (!_isMoving)
        _drawLockIn(_activeIndex);
}

void MenuView::onPress() { _dm->launchMenuItem(_activeIndex); }
void MenuView::onLongPress() {}
void MenuView::onDoublePress() {}