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

## File Structure

```
.
├── include/
├── lib/
├── platformio.ini
├── PROJECT_LOG.md
└── src/
    ├── config.h
    ├── main.cpp
    ├── api/
    │   ├── APIClient.h
    │   └── APIClient.cpp
    ├── data/
    │   ├── DataCache.h
    │   └── DataCache.cpp
    ├── display/
    │   ├── assets/          ← pre-converted driver headshot bitmaps (planned)
    │   ├── DisplayManager.h
    │   └── DisplayManager.cpp
    └── input/
        ├── EncoderInput.h
        └── EncoderInput.cpp
```

---

## Module Status

| Module | File(s) | Status | Notes |
|---|---|---|---|
| Config | `config.h` | ✅ Done | All pin definitions, constants set |
| Encoder input | `input/EncoderInput.*` | ✅ Done | Wired, ISR set up, callbacks working |
| Display manager | `display/DisplayManager.*` | 🟡 Placeholder | Init + basic text/number helpers only, no views |
| Data cache | `data/DataCache.*` | 🟡 Placeholder | Structs and manager defined, no data yet |
| API client | `api/APIClient.*` | 🟡 Placeholder | HTTP scaffolding done, fetch methods are stubs |
| Main | `main.cpp` | 🟡 In progress | Currently encoder test only |
| Display assets | `display/assets/` | ⬜ Not started | Driver headshots, pre-converted to RGB565 |
| WiFiManager setup | — | ⬜ Not started | Captive portal AP on first boot |

---

## Key Design Decisions

- **Post-session data only** — no live telemetry. OpenF1 REST API polled at a 10-minute interval (`API_UPDATE_INTERVAL = 600000`).
- **No LVGL** — rendering done directly with TFT_eSPI primitives for simplicity and memory.
- **Static driver assets** — headshots pre-converted offline to RGB565 bitmaps and stored in `display/assets/`, not fetched at runtime.
- **WiFiManager** — on first boot, device opens AP `"F1Widget"` for captive portal Wi-Fi config. Credentials persisted to flash.
- **Encoder acceleration disabled** — `setAcceleration(0)` for predictable 1-detent = 1-step navigation.
- **Encoder VCC pin = -1** — library constructor receives `-1` for VCC; physical VCC is hardwired to 3.3V.

---

## Task Plan

### ✅ Done
- [x] Hardware selection and pin mapping
- [x] `config.h` — all defines
- [x] `EncoderInput.h` / `.cpp` — ISR, init, loop, callbacks
- [x] `DisplayManager` — init, backlight, basic text/number draw
- [x] `DataCache` — struct definitions, manager class with getters/setters
- [x] `APIClient` — HTTP scaffolding, `makeRequest()` helper

### 🔧 In Progress
- [ ] `main.cpp` — encoder test running, integration not started

### ⬜ Up Next

#### WiFi
- [ ] Integrate WiFiManager into `setup()`
- [ ] Block until connected, show status on display

#### API
- [ ] Implement `fetchStandings()` — map OpenF1 driver standings endpoint to `DriverStanding[]`
- [ ] Implement `fetchResults()` — map last session results to `RaceResult[]`
- [ ] Implement `fetchSchedule()` — map upcoming sessions to `SessionSchedule[]`
- [ ] Wire `fetchAll()` into main loop with `API_UPDATE_INTERVAL` timer

#### Display — Views
- [ ] Design view system (enum-based page switching via encoder)
- [ ] Standings view — scrollable driver list, position + name + points
- [ ] Results view — last race podium / full grid
- [ ] Schedule view — next sessions with date + location
- [ ] Loading / splash screen

#### Display — Assets
- [ ] Select and crop driver headshots
- [ ] Convert to RGB565 bitmaps (Python script or image2cpp)
- [ ] Store in `display/assets/` as `.h` header arrays
- [ ] Integrate into standings/results views

#### Integration
- [ ] Connect encoder navigation to view switching
- [ ] Connect encoder press to sub-view or detail toggle
- [ ] End-to-end test: WiFi → fetch → display → navigate

#### Polish
- [ ] Stale data indicator (show last update time)
- [ ] Error screen for failed API fetch
- [ ] Brightness control (optional, via `TFT_BCKL` PWM)

---

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