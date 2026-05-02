#include <Arduino.h>
#include "LGFX_Config.h"
#include "display/DisplayManager.h"
#include "display/views/MenuView.h"
#include "display/views/DriverStandingsView.h"
#include "display/views/ConstructorStandingsView.h"
#include "display/views/CalendarView.h"
#include "display/views/NewsView.h"
#include "data/DataCache.h"
#include "api/APIClient.h"
#include "input/EncoderInput.h"
#include "wifi/WiFiManager.h"
#include "esp_system.h" // For ESP.getFreeHeap()
#include "LittleFS.h"

LGFX tft;
DisplayManager *dm = nullptr;
APIClient *api = nullptr;
MenuView *menuView = nullptr;
DriverStandingsView *driverView = nullptr;
ConstructorStandingsView *constructorView = nullptr;
CalendarView *calendarView = nullptr;
NewsView *newsView = nullptr;

// ── Encoder Callbacks ──────────────────────────────────────────
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
    delay(500);
    Serial.println("\n[BOOT] F1 Widget Starting...");

    // 1. Initialize Hardware (Screen First so we can show splash)
    tft.init();
    tft.setRotation(1);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    dm = new DisplayManager(&tft);
    dm->drawSplash(); // Show the logo while everything else loads!

    // 2. Initialize Filesystem & Cache
    if (!LittleFS.begin(true))
    {
        Serial.println("[ERROR] LittleFS Failed");
    }

    cache = new DataCache();
    cache->begin();

    // 3. Initialize WiFi and API
    api = new APIClient(cache);

    // This blocks until connected or timeout, but the user sees the Splash Screen
    if (wifi_init())
    {
        Serial.println("[BOOT] WiFi Connected. Syncing data...");
        api->syncAll();
    }
    else
    {
        Serial.println("[BOOT] WiFi Failed. Running in offline mode.");
    }

    // 4. Create and Register Views
    menuView = new MenuView(&tft, dm);
    driverView = new DriverStandingsView(&tft, dm);
    constructorView = new ConstructorStandingsView(&tft, dm);
    calendarView = new CalendarView(&tft, dm);
    newsView = new NewsView(&tft, dm);

    dm->registerView(MenuItem::DRIVER_STANDINGS, driverView);
    dm->registerView(MenuItem::CONSTRUCTOR_STANDINGS, constructorView);
    dm->registerView(MenuItem::CALENDAR, calendarView);
    dm->registerView(MenuItem::NEWS, newsView);

    // 5. Initialize Input
    encoder_init(-100, 100, 0);

    // 6. Launch
    Serial.println("[BOOT] Launching Menu...");
    dm->init(menuView);

    Serial.println("[BOOT] Setup Complete!");
}

void loop()
{
    encoder_loop();
    dm->loop();
}