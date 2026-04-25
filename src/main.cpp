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
#include "display/views/NewsView.h"

LGFX tft;
DisplayManager *dm = nullptr;
APIClient *api = nullptr;

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

// Views
MenuView *menuView = nullptr;
DriverStandingsView *driverView = nullptr;
ConstructorStandingsView *constructorView = nullptr;
NewsView *newsView = nullptr;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    if (!LittleFS.begin(true))
        Serial.println("FS Mount Failed");

    cache = new DataCache();
    cache->loadDrivers();
    cache->loadConstructors();

    tft.init();
    tft.setRotation(1);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH); // Backlight

    wifi_init();

    dm = new DisplayManager(&tft);
    api = new APIClient(cache);

    menuView = new MenuView(&tft, dm);
    driverView = new DriverStandingsView(&tft, dm);
    constructorView = new ConstructorStandingsView(&tft, dm);
    newsView = new NewsView(&tft, dm);

    // In setup()
    dm->registerView(0, driverView);
    dm->registerView(1, constructorView);
    dm->registerView(2, newsView);
    // dm->registerView(3, driversView);
    // dm->registerView(4, circuitsView);
    // dm->registerView(5, calendarView);
    // dm->registerView(6, settingsView);
    dm->init(menuView);

    encoder_init(-100, 100, 0);
    Serial.println("[System] Ready. Sync will start in 5s...");
}

void loop()
{
    encoder_loop();
    if (dm)
        dm->loop();

    // ── Delayed API Sync ──────────────────────────────────────────────────
    // Give the UI 5 seconds to stabilize before hitting the WiFi/RAM hard
    static bool firstSyncDone = false;
    if (!firstSyncDone && millis() > 5000)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("Pre-Sync Free RAM: %u\n", ESP.getFreeHeap());
            api->syncPersistentData();
            Serial.printf("Post-Sync Free RAM: %u\n", ESP.getFreeHeap());
        }
        firstSyncDone = true;
    }

    // Monitor RAM every 10 seconds to detect leaks
    static unsigned long lastMem = 0;
    if (millis() - lastMem > 10000)
    {
        Serial.printf("Current Free Heap: %u\n", ESP.getFreeHeap());
        lastMem = millis();
    }

    yield();
}