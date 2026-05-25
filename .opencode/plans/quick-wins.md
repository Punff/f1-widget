# Quick Wins Polish — Implementation Plan

## 1. Fix COL_F1_PURPLE (FL badge green)
**File:** `src/display/views/UI.h:38`
- `0x1FF8FF` → `0xF81F`
- `0x1FF8FF` is RGB(31,248,255) = cyan-green in 24-bit — explains green appearance
- `0xF81F` = R=31, G=0, B=31 in RGB565 = pure magenta, zero green

## 2. Fix double long-press navigation
**File:** `src/display/DisplayManager.cpp:136-140`
```cpp
// BEFORE:
void DisplayManager::onLongPress() {
    if (_header) _header->encoderLongPress();
    returnToMenu();
}

// AFTER:
void DisplayManager::onLongPress() {
    if (_header) _header->encoderLongPress();
    if (_currentView) _currentView->onLongPress();
}
```
**Files to add onLongPress() override:**
- `MenuView.h/.cpp` — add `void onLongPress() override { _dm->returnToMenu(); }`
- `DriverStandingsView.h/.cpp` — same
- `ConstructorStandingsView.h/.cpp` — same
- `CalendarView.h/.cpp` — same

**Views that already have onLongPress() (no change needed):**
- WeekendView: `_dm->returnToMenu()` — works
- SessionResultsView: `_dm->returnToMenu()` — works
- SettingsView: cancel edit on long-press — NOW ACTUALLY WORKS (was dead code before)
- NewsView: `_dm->returnToMenu()` — works

## 3. Deduplicate my_timegm()
**Create:** `src/time/TimeUtils.h`
```cpp
#pragma once
#include <time.h>

static inline time_t my_timegm(struct tm *t) {
    return t->tm_sec + t->tm_min*60 + t->tm_hour*3600 + t->tm_yday*86400 +
           (t->tm_year-70)*31536000 + ((t->tm_year-69)/4)*86400 -
           ((t->tm_year-1)/100)*86400 + ((t->tm_year+299)/400)*86400;
}
```
**Edit:** `CalendarView.cpp` — remove local `my_timegm()`, add `#include "../../time/TimeUtils.h"`
**Edit:** `WeekendView.cpp` — remove local `my_timegm()`, add `#include "../../time/TimeUtils.h"`

## 4. Fix encoder arc (3px, visible idle)
**File:** `src/display/views/Header.h`
```cpp
static constexpr int ARC_RMIN = 18;    // 18→21 = 3px thick (was 23→24 = 1px)
static constexpr int ARC_RMAX = 21;
static constexpr unsigned long COLOR_MS = 80;        // 80ms color burst (was 50ms)
static constexpr unsigned long WHITE_DECAY_MS = 400; // 400ms decay to dim (was 200ms)
```

**File:** `src/display/views/Header.cpp` — `drawEncoderArc()`
```cpp
void Header::drawEncoderArc() {
    unsigned long now = millis();
    unsigned long elapsed = now - _glowMs;

    if (elapsed < COLOR_MS) {
        // Phase 1: thick color burst
        _tft->fillArc(ARC_CX, ARC_CY, ARC_RMIN, ARC_RMAX, ARC_START, ARC_END, _glowColor);
    } else if (elapsed < WHITE_DECAY_MS) {
        // Phase 2: white flash — fades from full white to dim
        _tft->fillArc(ARC_CX, ARC_CY, ARC_RMIN, ARC_RMAX, ARC_START, ARC_END, UI::COL_TEXT);
    } else {
        // Phase 3: dim idle arc — always subtly visible
        _tft->fillArc(ARC_CX, ARC_CY, ARC_RMIN, ARC_RMAX, ARC_START, ARC_END, 0x444444);
    }
}
```
Also update `encoderActive()`: `return millis() - _glowMs < WHITE_DECAY_MS;` → returns true for 400ms after last input (was 200ms).

## 5. Brighten COL_BG_SEL
**File:** `src/display/views/UI.h:31`
- `0x1A1A1A` → `0x2A2A2A`
- TN panels crush near-black; 0x1A = 26/255 is barely distinguishable from 0

## 6. Zero-pad position numbers
**File:** `src/display/views/DriverStandingsView.cpp:59`
```cpp
// BEFORE:
_rowSprite->drawNumber(ds.position, COL_POS, _rowH / 2);
// AFTER:
char posBuf[4];
snprintf(posBuf, sizeof(posBuf), "%2d", ds.position);
_rowSprite->drawString(posBuf, COL_POS, _rowH / 2);
```

**File:** `src/display/views/ConstructorStandingsView.cpp:56`
```cpp
// BEFORE:
_rowSprite->drawNumber(cs.position, COL_POS, _rowH / 2);
// AFTER:
char posBuf[4];
snprintf(posBuf, sizeof(posBuf), "%2d", cs.position);
_rowSprite->drawString(posBuf, COL_POS, _rowH / 2);
```
