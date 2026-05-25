#include "Header.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"
#include "../../time/TimeManager.h"

Header::Header(LGFX *tft)
    : _tft(tft), _glowColor(0), _glowMs(0), _lastClockMin(-1) {}

void Header::draw(const char *title, const char *subtitle, const char *prefix)
{
    if (!prefix) prefix = "F1";

    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // 1. Prefix (Left)
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(prefix, H_PAD, H_CENTER_Y);

    // 2. Title/Subtitle (Middle)
    int titleX = 100;
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    if (subtitle) {
        _tft->drawString(title, titleX, H_CENTER_Y - 8);
        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString(subtitle, titleX, H_CENTER_Y + 12);
    } else {
        _tft->drawString(title, titleX, H_CENTER_Y);
    }

    // 3. Encoder Dot (Right)
    redrawEncoder();

    // 4. Divider
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);

    _lastClockMin = -1;
}

void Header::tick(TimeManager *tm)
{
    drawClock(tm);
    if (encoderActive())
        redrawEncoder();
}

void Header::drawClock(TimeManager *tm)
{
    if (!tm || !tm->isSynced()) return;

    time_t t = tm->getLocalTime();
    struct tm lt;
    localtime_r(&t, &lt);

    if (lt.tm_min == _lastClockMin) return;
    _lastClockMin = lt.tm_min;

    // Clock area: left of encoder dot
    int clockX = H_DOT_X - 15;
    _tft->fillRect(clockX - CLOCK_W, H_CENTER_Y - CLOCK_H/2, CLOCK_W, CLOCK_H, UI::COL_BG);

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
    _tft->setTextDatum(middle_right);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(buf, clockX, H_CENTER_Y);
}

void Header::encoderPulse(int dir) { _glowColor = UI::COL_F1_RED; _glowMs = millis(); redrawEncoder(); }
void Header::encoderPress() { _glowColor = UI::COL_F1_YELLOW; _glowMs = millis(); redrawEncoder(); }
void Header::encoderLongPress() { _glowColor = UI::COL_F1_RED; _glowMs = millis(); redrawEncoder(); }
void Header::redrawEncoder() { drawEncoderDot(); }
bool Header::encoderActive() const { return millis() - _glowMs < WHITE_DECAY_MS; }
void Header::markDirty() { _lastClockMin = -1; }

void Header::drawEncoderDot()
{
    unsigned long now = millis();
    unsigned long elapsed = now - _glowMs;

    uint32_t color;
    if (elapsed < COLOR_MS) color = _glowColor;
    else if (elapsed < WHITE_DECAY_MS) color = UI::COL_TEXT;
    else color = 0x222222;

    _tft->fillCircle(H_DOT_X, H_DOT_Y, H_DOT_R, color);
}
