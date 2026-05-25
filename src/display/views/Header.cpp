#include "Header.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"
#include "../../time/TimeManager.h"

Header::Header(LGFX *tft)
    : _tft(tft), _glowColor(0), _glowMs(0), _lastClockMin(-1) {}

void Header::draw(const char *title, const char *subtitle, const char *prefix)
{
    if (!prefix)
        prefix = "F1";

    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    int centerY = UI::HEADER_H / 2 - 2; // Slight upward offset for line divider

    _tft->setTextDatum(middle_left);
    _tft->setTextColor(UI::COL_F1_RED);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(prefix, 10, centerY);

    int titleX = 100; // Consistent indent
    if (subtitle)
    {
        _tft->setTextColor(UI::COL_TEXT);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString(title, titleX, centerY - 8);

        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString(subtitle, titleX, centerY + 12);
    }
    else
    {
        _tft->setTextColor(UI::COL_TEXT);
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString(title, titleX, centerY);
    }

    _tft->fillRect(0, UI::HEADER_H - 2, UI::SCREEN_W, 2, UI::COL_F1_RED);

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
    if (!tm || !tm->isSynced())
        return;

    time_t t = tm->getLocalTime();
    struct tm lt;
    localtime_r(&t, &lt);

    if (lt.tm_min == _lastClockMin)
        return;
    _lastClockMin = lt.tm_min;

    _tft->fillRect(CLOCK_RX - CLOCK_W, CLOCK_Y, CLOCK_W, CLOCK_H, UI::COL_BG);

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
    _tft->setTextDatum(top_right);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::BODY_MAIN);
    _tft->drawString(buf, CLOCK_RX, CLOCK_Y);
}

void Header::encoderPulse(int dir)
{
    _glowColor = UI::COL_F1_RED;
    _glowMs = millis();
    redrawEncoder();
}

void Header::encoderPress()
{
    _glowColor = UI::COL_F1_YELLOW;
    _glowMs = millis();
    redrawEncoder();
}

void Header::encoderLongPress()
{
    _glowColor = UI::COL_F1_RED;
    _glowMs = millis();
    redrawEncoder();
}

void Header::redrawEncoder()
{
    drawEncoderDot();
}

bool Header::encoderActive() const
{
    return millis() - _glowMs < WHITE_DECAY_MS;
}

void Header::markDirty()
{
    _lastClockMin = -1;
}

void Header::drawEncoderDot()
{
    unsigned long now = millis();
    unsigned long elapsed = now - _glowMs;

    if (elapsed < COLOR_MS)
    {
        _tft->fillCircle(DOT_X, DOT_Y, DOT_R, _glowColor);
    }
    else if (elapsed < WHITE_DECAY_MS)
    {
        _tft->fillCircle(DOT_X, DOT_Y, DOT_R, UI::COL_TEXT);
    }
    else
    {
        _tft->fillCircle(DOT_X, DOT_Y, DOT_R, 0x222222);
        _tft->drawCircle(DOT_X, DOT_Y, DOT_R + 1, 0x444444);
    }
}
