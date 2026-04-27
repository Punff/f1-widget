#include <Arduino.h>
#include <LittleFS.h>
#include "LGFX_Config.h"
#include "data/DataCache.h"
#include "api/APIClient.h"
#include "wifi/WiFiManager.h"
#include "input/EncoderInput.h"
#include "display/DisplayManager.h"
#include "display/views/MenuView.h"
#include "display/views/DriverStandingsView.h"
#include "display/views/ConstructorStandingsView.h"
#include "display/views/CalendarView.h"
#include "display/views/NewsView.h"

// Global instances
LGFX tft;
DataCache *cache = nullptr;
DisplayManager *dm = nullptr;
APIClient *api = nullptr;

// Views
MenuView *menuView = nullptr;
DriverStandingsView *driverView = nullptr;
ConstructorStandingsView *constructorView = nullptr;
CalendarView *calendar = nullptr;
NewsView *newsView = nullptr;

// ── Encoder Callbacks ──────────────────────────────────────────────────
void onEncoderTurnRight()
{
    if (dm)
        dm->onTurnRight();
}
void onEncoderTurnLeft()
{
    if (dm)
        dm->onTurnLeft();
}
void onEncoderPressed()
{
    if (dm)
        dm->onPress();
}
void onEncoderLongPress()
{
    if (dm)
        dm->onLongPress();
}
void onEncoderDoublePress()
{
    if (dm)
        dm->onDoublePress();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Initialize Cache and FS
    cache = new DataCache();
    cache->begin(); // Handles LittleFS.begin() and load() internally now

    // Initialize Display
    tft.init();
    tft.setRotation(1);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH); // Backlight

    // Network
    wifi_init();

    // Managers
    dm = new DisplayManager(&tft);
    api = new APIClient(cache);

    // Initialize Views
    menuView = new MenuView(&tft, dm);
    driverView = new DriverStandingsView(&tft, dm);
    constructorView = new ConstructorStandingsView(&tft, dm);
    calendar = new CalendarView(&tft, dm);
    newsView = new NewsView(&tft, dm);

    // Register Views to Manager
    dm->registerView(0, driverView);
    dm->registerView(1, constructorView);
    dm->registerView(2, calendar);
    dm->registerView(3, newsView);

    // Set default view
    dm->init(menuView);

    // Input
    encoder_init(-100, 100, 0);

    Serial.println("[System] Ready. Sync will start in 5s...");
}

void loop()
{
    encoder_loop();
    if (dm)
        dm->loop();

    // ── Delayed API Sync ──────────────────────────────────────────────────
    static bool firstSyncDone = false;
    if (!firstSyncDone && millis() > 5000)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("[API] Syncing... Free RAM: %u\n", ESP.getFreeHeap());

            if (api->syncAll())
            {
                Serial.println("[API] Sync Successful");

                // FIX: Access the current view from the manager and call render()
                if (dm && dm->currentView())
                {
                    dm->currentView()->render();
                }
            }
            else
            {
                Serial.println("[API] Sync Failed");
            }

            Serial.printf("[API] Sync Done. Free RAM: %u\n", ESP.getFreeHeap());
        }
        firstSyncDone = true;
    }

    // Monitor RAM every 10 seconds to detect leaks
    static unsigned long lastMem = 0;
    if (millis() - lastMem > 10000)
    {
        Serial.printf("System Heartbeat - Free Heap: %u\n", ESP.getFreeHeap());
        lastMem = millis();
    }

    yield();
}