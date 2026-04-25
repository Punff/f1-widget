# F1 Widget — Project Log

> Living document for agent handoff and progress tracking.
> Update this file whenever a module is changed, completed, or redesigned.

---

## Project Overview

An ESP32-based Formula 1 data display widget. Fetches post-session data from the OpenF1 REST API and displays standings, race results, and schedule on a 3.5" TFT screen. Navigation via rotary encoder. Always-on, USB-C powered.

---

## Hardware

| Component | Details |
|---|---|
| MCU board | ESP32-3248S035C |
| Display | 3.5" TN TFT, ST7796 driver, 480×320 |
| Input | KY-040 rotary encoder |
| Power | USB-C, always-on |

### Encoder Wiring

| Encoder Pin | GPIO | Notes |
|---|---|---|
| CLK (A) | 22 | Via P3 port |
| DT (B) | 21 | Via P3 port |
| SW (Button) | 35 | Input-only, no internal pull-up — module VCC handles pull-up |
| VCC | 3.3V | Wired directly to module VCC pin |
| GND | GND | |

> GPIO35 is input-only on this board. The onboard pull-up resistor on the KY-040 module handles SW correctly when VCC is supplied via the module's VCC pin. No external resistor needed.

---

## Software Stack

| Layer | Choice |
|---|---|
| Framework | Arduino (PlatformIO) |
| Display library | TFT_eSPI |
| Encoder library | AiEsp32RotaryEncoder |
| HTTP | HTTPClient (ESP32 Arduino core) |
| JSON | ArduinoJson |
| WiFi config | WiFiManager (captive portal AP) |
| API | OpenF1 REST API (post-session, no WebSocket) |

### Dev Environment
- OS: CachyOS + Niri compositor
- IDE: VSCode + PlatformIO extension

---
Architecture Overview
Encoder-centric design: a persistent vertical strip on the right edge (~40px wide) renders the virtual encoder widget at all times. Everything else renders in the remaining 440×320 space. No view ever draws into the encoder strip.

Screen Layout
┌─────────────────────────────────────────┬──────┐
│                                         │  ↑   │
│           VIEW CONTENT AREA             │  █   │
│              440 × 320                  │  █   │
│                                         │  ↓   │
└─────────────────────────────────────────┴──────┘
                                          40px strip
The encoder widget reacts visually to every input — rotation animates the dial, press flashes it, long press shows a "home" indicator.

File Structure
src/
├── config.h
├── main.cpp
├── input/
│   ├── EncoderInput.h
│   └── EncoderInput.cpp
├── wifi/
│   ├── WiFiManager.h
│   └── WiFiManager.cpp
├── display/
│   ├── DisplayManager.h
│   ├── DisplayManager.cpp
│   ├── EncoderWidget.h
│   ├── EncoderWidget.cpp
│   ├── IView.h
│   └── views/
│       ├── MenuView.h
│       ├── MenuView.cpp
│       ├── DriverStandingsView.h
│       ├── DriverStandingsView.cpp
│       ├── ConstructorStandingsView.h
│       ├── ConstructorStandingsView.cpp
│       └── NewsView.h
│           NewsView.cpp
├── api/
│   ├── APIClient.h
│   └── APIClient.cpp
└── data/
    ├── DataCache.h
    └── DataCache.cpp

Storage Partitioning Plan
LittleFS partition (default 1MB on esp32dev, increase to 1.5MB in partitions.csv if headshots are added later):
/drivers.json        — driver standings array, written after every fetch
/constructors.json   — constructor standings array
/news.json           — articles array
/last_update.json    — timestamps per data type
/settings.json       — user preferences (brightness, update interval etc) — future
What gets saved and when:
DataSaved whenLoaded whenDriver standingsAfter successful API fetchBoot, before first renderConstructor standingsAfter successful API fetchBoot, before first renderNews articlesAfter successful RSS fetchBoot, before first renderLast update timestampsAfter each successful fetchBoot, to decide if fetch neededSettingsWhen user changes a settingBootWi-Fi credentialsBy WiFiManager automaticallyBoot
Update logic: On boot, load from flash and render immediately (no waiting for API). Then check timestamps — if stale (>24h default, configurable), trigger fetchAll() in background. On success, update flash and re-render affected views.

config.h additions needed
cpp// Storage
#define DRIVERS_FILE       "/drivers.json"
#define CONSTRUCTORS_FILE  "/constructors.json"
#define NEWS_FILE          "/news.json"
#define LAST_UPDATE_FILE   "/last_update.json"
#define SETTINGS_FILE      "/settings.json"

// API
#define API_UPDATE_INTERVAL_MS  86400000UL   // 24h default
#define API_SEASON              "2024"        // update each season

// Display
#define SCREEN_W         480
#define SCREEN_H         320
#define CONTENT_W        440   // screen minus encoder strip
#define ENCODER_STRIP_X  440
#define ENCODER_STRIP_W  40

What's Left — Full Task Order
✅ Encoder input (long press, double press, single press)
✅ WiFiManager (captive portal, credentials to flash)

── DISPLAY ──────────────────────────────────────────
[ ] EncoderWidget — draw strip, idle + react states
[ ] DisplayManager — init, setView, input delegation
[ ] MenuView — carousel render + transition animation
[ ] Wire encoder callbacks in main.cpp → DisplayManager

── DATA ─────────────────────────────────────────────
[ ] DataCache — implement load/save/stale per data type
[ ] APIClient — fetchDriverStandings(), parse, save to cache
[ ] APIClient — fetchConstructorStandings(), parse, save
[ ] APIClient — fetchNews() RSS parse, save

── VIEWS ────────────────────────────────────────────
[ ] DriverStandingsView — render from cache, scroll
[ ] ConstructorStandingsView — render from cache, scroll
[ ] NewsView — render from cache, page through articles

── INTEGRATION ──────────────────────────────────────
[ ] Boot sequence — load cache → connect WiFi → fetch if stale → render
[ ] Stale data indicator in views (show last update time)
[ ] Error screen — no WiFi / fetch failed

── POLISH ───────────────────────────────────────────
[ ] Splash screen on boot
[ ] Settings view (brightness, update interval, force refresh)
[ ] Driver headshot assets — convert RGB565, integrate into standings
[ ] Tracks view
[ ] Calendar view
[ ] Weekend view (next race countdown + session schedule)

## API Notes (OpenF1)

Base URL: `https://api.openf1.org/v1`

Useful endpoints (to be confirmed during implementation):
- `/drivers` — driver info by session
- `/position` — finishing positions
- `/sessions` — session list with dates and locations

OpenF1 is session-based, not season-standings-based. Driver championship standings will likely require aggregation across session results, or a workaround. **This needs investigation before implementing `fetchStandings()`.**

---

## Change Log

| Date | Change | Files touched |
|---|---|---|
| 2026-04-09 | Initial scaffold — all modules created as stubs | all |
| 2026-04-09 | Encoder wiring finalised (VCC pin added), ISR and callbacks implemented | `EncoderInput.*`, `config.h` |
| 2026-04-09 | PROJECT_LOG.md created | `PROJECT_LOG.md` |