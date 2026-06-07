#include "Header.h"
#include "UI.h"
#include "../../../include/UI_Fonts.h"
#include "../../../include/LGFX_Config.h"
#include "../../time/TimeManager.h"

Header::Header(LGFX *tft)
    : _tft(tft), _glowColor(0), _glowMs(0), _lastClockMin(-1), _lastClockHour(-1), _clockStale(false) {}

void Header::draw(const char *title, const char *subtitle, const char *prefix)
{
    if (!prefix) prefix = "F1";

    _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);

    // 1. Prefix badge (Left) — measure width to position title dynamically
    _tft->setTextDatum(middle_left);
    _tft->setTextColor(UI::COL_ACCENT);
    _tft->setFont(UI::Fonts::HEADER_BIG);
    _tft->drawString(prefix, H_PAD, H_CENTER_Y);
    int titleX = H_PAD + _tft->textWidth(prefix) + 12;
    if (titleX < 80) titleX = 80;

    // 2. Title/Subtitle
    _tft->setTextColor(UI::COL_TEXT);
    if (subtitle && strlen(subtitle) > 0) {
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString(title, titleX, H_CENTER_Y - 8);

        _tft->setTextColor(UI::COL_MUTED);
        _tft->setFont(UI::Fonts::LABEL_SMALL);
        _tft->drawString(subtitle, titleX, H_CENTER_Y + 12);
    } else {
        _tft->setFont(UI::Fonts::BODY_MAIN);
        _tft->drawString(title, titleX, H_CENTER_Y);
    }

    // 3. Encoder Dot (Right)
    redrawEncoder();

    // 4. Divider
    _tft->drawFastHLine(0, UI::HEADER_H - 2, UI::SCREEN_W, UI::COL_ACCENT);
    _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_ACCENT);

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
    if (!tm) return;

    bool synced = tm->isSynced();
    time_t t = tm->getLocalTime();
    struct tm lt;
    localtime_r(&t, &lt);

    if (synced) {
        if (lt.tm_min == _lastClockMin && !_clockStale) return;
        _lastClockMin = lt.tm_min;
        _lastClockHour = lt.tm_hour;
        _clockStale = false;
    } else {
        if (_clockStale) return;
        _clockStale = true;
    }

    int clockX = H_DOT_X - 15;
    _tft->fillRect(clockX - CLOCK_W, H_CENTER_Y - CLOCK_H/2, CLOCK_W, CLOCK_H, UI::COL_BG);

    char buf[6];
    if (_lastClockHour == -1) snprintf(buf, sizeof(buf), "--:--");
    else snprintf(buf, sizeof(buf), "%02d:%02d", _lastClockHour, _lastClockMin);
    
    _tft->setTextDatum(middle_right);
    _tft->setTextColor(UI::COL_TEXT);
    _tft->setFont(UI::Fonts::LABEL_SMALL);
    _tft->drawString(buf, clockX, H_CENTER_Y);
}

void Header::encoderPulse(int dir) { _glowColor = UI::COL_ACCENT; _glowMs = millis(); redrawEncoder(); }
void Header::encoderPress() { _glowColor = UI::COL_F1_YELLOW; _glowMs = millis(); redrawEncoder(); }
void Header::encoderLongPress() { _glowColor = UI::COL_ACCENT; _glowMs = millis(); redrawEncoder(); }
void Header::redrawEncoder() { drawEncoderDot(); }
bool Header::encoderActive() const { return millis() - _glowMs < WHITE_DECAY_MS; }
void Header::markDirty() { _lastClockMin = -1; _clockStale = false; }

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
