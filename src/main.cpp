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
#include "time/TimeManager.h"
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
TimeManager *timeMgr = nullptr;

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

    // 1. Initialize Hardware
    tft.init();
    tft.setRotation(1);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    // 2. Mount LittleFS ONCE
    if (!LittleFS.begin(true))
    {
        Serial.println("[ERROR] LittleFS Failed");
    }
    Serial.println("[BOOT] LittleFS mounted");

    // 3. Initialize Cache (LittleFS already mounted)
    cache = new DataCache();
    cache->begin();
    Serial.printf("[BOOT] Cache loaded, calendar has %d races\n", cache->calendar.size());

    // 4. Create DisplayManager and show splash (LittleFS already mounted)
    dm = new DisplayManager(&tft);
    dm->drawSplash();

    // 5. Initialize API client
    api = new APIClient(cache);

    // 6. Connect WiFi and sync (blocking until connected)
    if (wifi_init())
    {
        Serial.println("[BOOT] WiFi Connected. Syncing data...");
        api->syncAll();
        Serial.printf("[BOOT] After sync, calendar has %d races\n", cache->calendar.size());

        // Initialize TimeManager and sync NTP
        timeMgr = new TimeManager();
        if (!timeMgr->syncNTP()) {
            Serial.println("[BOOT] NTP sync failed, retrying...");
            delay(2000);
            timeMgr->syncNTP();
        }
    }
    else
    {
        Serial.println("[BOOT] WiFi Failed. Running in offline mode.");
    }

    // 7. Create and Register Views
    menuView = new MenuView(&tft, dm);
    driverView = new DriverStandingsView(&tft, dm);
    constructorView = new ConstructorStandingsView(&tft, dm);
    calendarView = new CalendarView(&tft, dm);
    newsView = new NewsView(&tft, dm);

    dm->registerView(MenuItem::DRIVER_STANDINGS, driverView);
    dm->registerView(MenuItem::CONSTRUCTOR_STANDINGS, constructorView);
    dm->registerView(MenuItem::CALENDAR, calendarView);
    dm->registerView(MenuItem::NEWS, newsView);

    // 8. Initialize Input
    encoder_init(-100, 100, 0);

    // 9. Launch Menu
    Serial.println("[BOOT] Launching Menu...");
    dm->init(menuView);

    Serial.println("[BOOT] Setup Complete!");
}

void loop()
{
    encoder_loop();
    dm->loop();
}
