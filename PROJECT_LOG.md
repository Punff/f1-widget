# F1 Embedded Widget OS: Core Architecture & UI/UX Specification

**Target Hardware:** ESP32 / 480x320 TN Panel TFT via LovyanGFX / Rotary Encoder with Click Button

---

## 1. System Architecture & Display Engine Rules

To achieve a premium, responsive feel on an ESP32 driving a slow-refresh TN panel over SPI, agents must write code conforming to the strict pipeline rules detailed below.

### The Universal Frame Grid (480x320)

The screen area is explicitly divided into three vertical segments. The Header and Footer are universal constraints managed globally by the system, ensuring visual stability while content frames swap out underneath.

```
+-------------------------------------------------------+  Y: 0
| HEADER: Title [75px wide] | Context | [Visualizer]    |  Height: 30px
+-------------------------------------------------------+  Y: 30
|                                                       |
|                                                       |
| CONTENT VIEWPORT                                      |  Height: 265px
|                                                       |
|                                                       |
+-------------------------------------------------------+  Y: 295
| FOOTER: Tooltips & Dynamic Telemetry Array           |  Height: 25px
+-------------------------------------------------------+  Y: 320
X: 0                                                    X: 480

```

### The Sprite Engine & Anti-Flicker Protocols

* **Zero Direct TFT Writes during View States:** Child views must never call `_tft->fillScreen()` or draw primitive elements directly to the panel during interaction updates. This prevents heavy screen tearing and white-out flashes.
* **Row-Buffered Memory Footprint:** Allocating a full 480x265 16-bit color sprite in memory requires ~254 KB of RAM, which chokes the ESP32’s available heap. Instead, agents must use a sliding row-buffer system (`LGFX_Sprite _rowSprite`). Sprites are allocated per row (e.g., $480 \times \text{Row Height}$). Each row is drawn completely in off-screen RAM and pushed to its precise Y-offset coordinate on the TFT using `pushSprite(0, rowY)`.
* **Defensive State Resets:** The `_rowSprite` is a shared asset. Before any child view executes its row rendering logic, `ScrollListView::drawSingleRow` must run a state purification routine:
```cpp
_rowSprite->fillSprite(UI::COL_BG);
_rowSprite->setTextColor(UI::COL_TEXT, UI::COL_BG);
_rowSprite->setTextDatum(top_left);
_rowSprite->setCursor(0, 0);

```



### The Encoder Visualizer

Located in the far top-right corner of the universal header ($X: 440 \text{ to } 475, Y: 5 \text{ to } 25$).

* **Mechanics:** A dedicated, ultra-lightweight micro-sprite. It renders a minimal, high-refresh graphic—a clean, segmented bar or a spinning bracket—that animates matching the hardware interrupt clicks of the rotary encoder.
* **UX Purpose:** Provides immediate, zero-latency physical validation to the user while the heavier content view is processing calculations or waiting on SPI bus availability.

---

## 2. Core Design System & Aesthetic Intent

The user interface must look and feel like a custom piece of professional motorsport telemetry gear, not a generic open-source microcontroller project.

### Color Palette & Panel Optimization

TN panels wash out highlights and compress dark ranges if viewing angles deviate. The palette relies on flat, aggressive, high-contrast values:

* **`UI::COL_BG` (Deep Slate Black / `0x0841`):** Absolute dark background to minimize TN panel bleed.
* **`UI::COL_F1_RED` (Neon Racing Red / `0xE000`):** Used exclusively for structural accents, selector highlights, and critical states.
* **`UI::COL_TEXT` (Pure Stark White / `0xFFFF`):** High-readability layer for primary telemetry numbers and driver codes.
* **`UI::COL_MUTED` (Chassis Grey / `0x7BEF`):** Secondary data markers, boundaries, and static table columns.
* **`UI::COL_ACCENT` (Podium Yellow / `0xF6A0`):** Used strictly for active qualifiers, live-session green sectors, or system notification flags.

### Typography Hierarchy

All rendering modules must explicitly link fonts to these semantic definitions:

* **`UI::Fonts::HEADER_BIG`:** Custom, high-impact sans-serif built specifically to mirror standard motorsport branding. Maximize kerning; uppercase only.
* **`UI::Fonts::DATA_ACCENT`:** Hard-edged, monospaced font for lap times, points, position counters, and racing numbers. Prevents layout shifting when values increment.
* **`UI::Fonts::BODY_MAIN`:** Clean, highly compressed, mid-size sans-serif optimizing text density per row.

---

## 3. Data Lifecycle & Cache Volatility Tiering

To maximize UI snappiness, network requests must never occur on the main execution thread of a view during interactions. Data layers are split into four clear tiers managed by `DataCache` and `APIClient`.

```
[Static Tier: Boot/24h] -------> Circuit Vectors, Driver Bios, Core Calendars
[Slow Tier: 1h Interval] ------> Season Standings, Team Constructor Points
[Medium Tier: 30m Interval] ---> News Feed Summaries, Headline Arrays
[Fast Tier: 1m to Live] -------> Live Session Sectors, Active Track Status

```

1. **Static Tier (Fetch: On Boot / 24-Hour Loop)**
* *Payloads:* Complete annual race calendars, circuit telemetry profiles (lap records, coordinates), driver biographical cards.
* *Storage:* Parsed into RAM on initialization, written down to `LittleFS` cache JSON arrays.


2. **Slow Tier (Fetch: 1-Hour Interval)**
* *Payloads:* Driver Championship standings, Constructor Point tallies.
* *Behavior:* Polled every hour; updates to a 5-minute interval only on Sunday afternoon if a race is flagged active.


3. **Medium Tier (Fetch: 30-Minute Interval)**
* *Payloads:* RSS/API media headlines, official press wire feeds.


4. **Fast Tier (Fetch: 1-Minute Interval to Real-Time Streaming)**
* *Payloads:* Live timing loops, active session position ladders, current circuit track temperatures.
* *Context Logic:* This interval is completely deactivated on non-race weekends. It transitions to active status only when `TimeManager` matches a time block defined within the Static Calendar array.



---

## 4. Universal Interaction Matrix & State Machine

The rotary encoder uses uniform global controls across all software views to build consistent muscle memory.

```
                  ┌──────────────────────┐
                  │       MAIN MENU      │
                  └──────────────────────┘
                             ▲
                             │ (Long Press - System Escape Hatch)
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│                           ANY VIEW STATE                         │
├──────────────────────────────────────────────────────────────────┤
│ Rotate Knob  ──► Increments List Selection / Horizontal Pan      │
│ Short Press  ──► Executes Activation / Drill-down into child view│
│ Double Press ──► Contextual Back / Clears secondary modal        │
└──────────────────────────────────────────────────────────────────┘

```

### Navigation Map & View-to-View Transitions

* **Main Menu** $\rightarrow$ *Short Press* $\rightarrow$ **Launches targeted view state.**
* **Calendar View** $\rightarrow$ *Short Press on Row* $\rightarrow$ **Launches Weekend View** loaded with that row’s specific `RaceMeeting*` pointer.
* **Driver Standings** $\rightarrow$ *Short Press on Row* $\rightarrow$ **Launches Driver Poster View** loaded with that specific driver's profile vector.
* **Team Standings** $\rightarrow$ *Short Press on Row* $\rightarrow$ **Launches Team Poster View** loaded with constructor specifications.
* **Any View** $\rightarrow$ *Long Press* $\rightarrow$ **Intercepted instantly at base class level; flushes current display context and drops user back to Main Menu.**

---

## 5. Comprehensive View Specifications

### View 1: Main Menu System

* **Aesthetic Feel:** Minimalist dashboard hub. Smooth, deliberate, and highly optimized for minimal screen-draw overhead.
* **UI Design:** Vertical icon-and-text carousel aligned down a left-justified axis ($X: 30$). The currently active item is locked to the center line of the screen. Non-active items fade in scale and color brightness (`UI::dimCol` tracking with distance from center). A vertical accent line in `UI::COL_F1_RED` frames the selection area.
* **Functional Logic:** Uses an indexed array of view tokens. Uses partial row-sprite overrides during scrolling so that only the entry moving out of center and the entry moving into center redraw.
* **Encoder Handling:**
* *Rotate:* Scrolls selection.
* *Short Press:* Jumps directly to the assigned view token.
* *Double Press:* No action.
* *Long Press:* Re-initializes current menu view baseline.



### View 2: Driver Standings View

* **Aesthetic Feel:** Highly dense, clean, and numbers-focused data grid.
* **UI Design:** * *Header:* "DRIVER STANDINGS" | Left: Season Year, Right: System Time.
* *Content Grid:* Row height 36px. 6 rows visible simultaneously.
* Column 1 ($X: 10$): Position Rank (Large, `UI::Fonts::DATA_ACCENT`).
* Column 2 ($X: 50$): 3-Letter Driver Code (e.g., "VER", "HAM") in bold white text, followed by the team name in muted grey.
* Column 3 ($X: 430$): Total championship points, right-aligned to match standard leaderboard formats.
* *Row Highlights:* Current selection gets a sharp 4px red left border block.


* **Functional Logic:** Pulls array elements from `cache->driverStandings`. Calculates the distance from the currently highlighted entry to apply a fading brightness calculation (`rowBrightness(dist)`).
* **Footer Telemetry:** Displays the point margin to the current leader for the driver under the cursor: `GAP TO LEADER: -42.5 PTS`.
* **Encoder Handling:**
* *Rotate:* Moves selection cursor through the driver listings.
* *Short Press:* Reads selected driver's index and passes context to launch the **Driver Poster View**.
* *Double Press:* Returns to Main Menu.



### View 3: Team Standings View

* **Aesthetic Feel:** Bold corporate racing team grid tracking constructor points.
* **UI Design:** Similar structure to the Driver Standings View, but row heights expand to 44px to display larger elements.
* Column 1 ($X: 15$): Position rank.
* Column 2 ($X: 60$): Full Team Constructor title.
* Column 3 ($X: 420$): Points tally.
* *Visual Accent:* A colored horizontal block matching the team's official livery color runs under the team name text ($X: 60 \text{ to } 200, Y: \text{Row Base} - 4$).


* **Functional Logic:** Binds data directly from `cache->constructorStandings`.
* **Footer Telemetry:** `LEADER GAP: [X] PTS` updated relative to row position.
* **Encoder Handling:**
* *Rotate:* Changes selected constructor team row.
* *Short Press:* Expands a secondary pop-up card displaying data for both team drivers side-by-side.
* *Double Press:* Closes driver pop-up if visible; otherwise returns to Main Menu.



### View 4: News Feed View

* **Aesthetic Feel:** High-legibility digital reader layout.
* **UI Design:** Avoids traditional smooth vertical text scrolling, which turns low-cost TN panel text into an unreadable blur. Instead, uses a clean "Page-Flip" system.
* *Content Area:* Main text bounding box ($440 \times 220$). Displays a large, clean news headline in bold white text, followed by a horizontal divider line and a 4-line article summary block.


* **Functional Logic:** News arrays are split into discrete multi-line page tokens during view entry.
* **Footer Telemetry:** Bottom bar displays current reading progress indicators: `STORY 02 / 10  |  PAGE 1/1`.
* **Encoder Handling:**
* *Rotate:* Directly loads the next or previous news story card.
* *Short Press:* Toggles the summary box between condensed mode and full-screen view.
* *Double Press:* Drops out back to Main Menu.



### View 5: Current Weekend Info View

* **Aesthetic Feel:** High-priority mission dashboard tracking live and upcoming race schedules.
* **UI Design:** * *Header:* Displays official race venue name and current local track track status flags (e.g., "TRACK: GREEN", "TRACK: SC").
* *Content Layout:* Split into 2 primary display columns.
* Left Column ($X: 10 \text{ to } 260$): Schedule breakdown grid tracking standard sessions (FP1, FP2, FP3, Quali, Grand Prix). Displays session names and real-world start times automatically shifted to match the user's localized time settings.
* Right Column ($X: 280 \text{ to } 470$): Real-time live summary block. If a session goes live, this canvas switches to show live track telemetry: the session timer running down and a live leaderboard tracking the current top 3 drivers.


* **Functional Logic:** Uses an active checking loop tracking system time via `TimeManager`. If current time matches an active track session window, the UI automatically transitions to high-frequency live tracking mode.
* **Footer Telemetry:** Alternates displays between `NTP TIME SYNC VALID` and current weather conditions.
* **Encoder Handling:**
* *Rotate:* Navigates focus between scheduled weekend sessions.
* *Short Press:* If a session is completed and data is cached, opens the final classification list.
* *Double Press:* Returns to the standard Calendar overview.



### View 6: Calendar View

* **Aesthetic Feel:** Linear season tracking view showing historical records and upcoming events.
* **UI Design:** Clean list architecture tracking consecutive schedule rounds.
* *Row States:*
1. Past Races: Complete text block dims to grey (`UI::COL_TEXT_DIM`), trailing indicator displays a clear `DONE` stamp.
2. Active/Upcoming Race: The row glows with a subtle dark red background tint (`0x1800`), standard text displays in high-contrast white, and an explicit red indicator tag marks it as `NEXT UP`.




* **Functional Logic:** On entering the view, the system checks the `TimeManager` Unix clock against cached calendar values to find the next unplayed race. The interface bypasses old entries and automatically scrolls to lock the next upcoming race to the screen center.
* **Footer Telemetry:** Displays exact remaining countdown metrics: `FP1 LIGHTS OUT IN: DD:HH:MM`.
* **Encoder Handling:**
* *Rotate:* Moves selection through the season rounds.
* *Short Press:* Selects a race and opens its detailed **Current Weekend Info View**.
* *Double Press:* Instantly snaps focus back to center on the active `NEXT UP` race row.



### View 7: Drivers Showcase View

* **Aesthetic Feel:** High-impact digital profile card layout.
* **UI Design:** Non-scroll profile poster layout split into two distinct visual areas.
* Left Column ($X: 0 \text{ to } 200$): Reserved for loading driver portrait imagery. Reads raw binary uncompressed bitmap assets directly out of `LittleFS` partition storage (e.g., `/img/drivers/44.bmp`).
* Right Column ($X: 215 \text{ to } 480$): Core racing telemetry card. Displays large driver number in team colors, full driver name, country flag, current world ranking points, podium counts, and career data points.


* **Functional Logic:** Avoids smooth scrolling transitions to prevent panel ghosting. Rotating the encoder switches drivers completely, performing a fast data swap and clean frame redraw.
* **Footer Telemetry:** `TOOLTIPS: ⟳ CYCLE PROFILE | ⚬ EXIT TO MENU`.
* **Encoder Handling:**
* *Rotate:* Switches view to load the next or previous driver profile card.
* *Short Press:* Flips card content area to show a brief biographical summary.
* *Double Press:* Exits to Main Menu.



### View 8: Circuits Showcase View

* **Aesthetic Feel:** Clean structural telemetry dashboard focusing on track layouts.
* **UI Design:** Similar layout architecture to the Drivers Showcase View.
* Left Column ($X: 10 \text{ to } 240$): High-contrast vector outline tracking the circuit track profile geometry.
* Right Column ($X: 260 \text{ to } 480$): Technical specifications layout block. Text details official track name, length in kilometers, total race lap counts, DRS activation zones, and the standing all-time official lap record holder.


* **Functional Logic:** Reads vector track arrays directly from memory arrays and renders coordinates cleanly onto an isolated left side sprite block.
* **Footer Telemetry:** Displays current selected venue coordinates or local time zone offsets.
* **Encoder Handling:**
* *Rotate:* Switches view layout to load the next circuit profile.
* *Short Press:* Redraws left canvas area to highlight exactly where DRS tracking zones are physically located on the track vector.
* *Double Press:* Exits to Main Menu.



### View 9: Settings View

* **Aesthetic Feel:** Clean, functional system engineering panel.
* **UI Design:** Crisp vertical list layout tracking device variables.
* *Row Configurations:* Each item displays its setting title left-justified and its current runtime value right-justified (e.g., `BRIGHTNESS .......... [ 80% ]`).


* **Functional Logic:** Modifying any row variable commits changes to local system memory and triggers live hardware updates. Selecting the Brightness row and turning the dial passes value updates directly to the device PWM backlight controller pins.
* **Footer Telemetry:** Displays hardware diagnostic readouts: `HEAP FREE: XXX KB | RSSI: -XX dBm`.
* **Encoder Handling:**
* *Rotate:* Navigates through system menu options.
* *Short Press:* Selects a row item and locks focus onto its value field (value text flashes red to indicate active modification mode). Turning the encoder now modifies the variable. Pressing again saves changes and exits modification mode.
* *Double Press:* Clears active modification state without saving; if inactive, returns to Main Menu.



---

## 6. Implementation Code Template for Reference

Agents implementing new views must strictly follow this structural wrapper pattern to ensure system safety and interaction uniformity:

```cpp
#include "ScrollListView.h"
#include "../../data/DataCache.h"
#include "../../time/TimeManager.h"
#include "../DisplayManager.h"
#include "../../../include/UI_Fonts.h"

class GenericTelemetryView : public ScrollListView {
public:
    GenericTelemetryView(LGFX *tft, DisplayManager *dm) 
        : ScrollListView(tft, dm, 40, 5, 2) {}

protected:
    int dataSize() const override { 
        return cache->targetDataArray.size(); 
    }

    void onEnter() override {
        // Enforce data integrity verification prior to rendering pipelines
        if(cache->targetDataArray.empty()) {
            return;
        }
        _cursor = 0;
        ScrollListView::onEnter();
    }

    void drawHeader() override {
        _tft->fillRect(0, 0, UI::SCREEN_W, UI::HEADER_H, UI::COL_BG);
        _tft->setTextColor(UI::COL_F1_RED);
        _tft->setFont(UI::Fonts::HEADER_BIG);
        _tft->drawString("TELEMETRY", 10, 8);
        _tft->drawFastHLine(0, UI::HEADER_H - 1, UI::SCREEN_W, UI::COL_F1_RED);
    }

    void drawRow(int dataIdx, bool selected, int dist) override {
        // Safe data guard boundary
        if (dataIdx < 0 || dataIdx >= dataSize()) return;
        
        float brightness = rowBrightness(dist);
        uint32_t activeColor = selected ? UI::COL_TEXT : dimCol(UI::COL_TEXT, brightness);
        
        _rowSprite->setTextDatum(middle_left);
        _rowSprite->setFont(UI::Fonts::BODY_MAIN);
        _rowSprite->setTextColor(activeColor);
        _rowSprite->drawString(cache->targetDataArray[dataIdx].name, 15, _rowH / 2);
    }

    void drawFooter() override {
        _tft->fillRect(0, UI::FOOTER_Y, UI::SCREEN_W, UI::FOOTER_H, UI::COL_BG);
        _tft->drawFastHLine(0, UI::FOOTER_Y, UI::SCREEN_W, UI::COL_F1_RED);
        
        // Execute optimized text buffering routines
        if (_footerTextChanged("SYSTEM READY")) {
            drawFooterText("SYSTEM READY", 470, UI::FOOTER_Y + (UI::FOOTER_H/2), UI::COL_MUTED, 1);
        }
    }
};

```