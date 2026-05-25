#include <Arduino.h>
#include "LGFX_Config.h"
#include "display/DisplayManager.h"
#include "display/views/MenuView.h"
#include "display/views/DriverStandingsView.h"
#include "display/views/ConstructorStandingsView.h"
#include "display/views/CalendarView.h"
#include "display/views/NewsView.h"
#include "display/views/SettingsView.h"
#include "data/DataCache.h"
#include "api/APIClient.h"
#include "input/EncoderInput.h"
#include "wifi/WiFiManager.h"
#include "time/TimeManager.h"
#include "esp_system.h"
#include "LittleFS.h"

LGFX tft;
DisplayManager *dm = nullptr;
APIClient *api = nullptr;
MenuView *menuView = nullptr;
DriverStandingsView *driverView = nullptr;
ConstructorStandingsView *constructorView = nullptr;
CalendarView *calendarView = nullptr;
NewsView *newsView = nullptr;
SettingsView *settingsView = nullptr;
TimeManager *timeMgr = nullptr;

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
    Serial.printf("\n[BOOT] F1 Widget Starting... Free heap: %u bytes\n", ESP.getFreeHeap());

    tft.init();
    tft.initDMA();
    tft.setRotation(1);

    // Backlight PWM init (5000Hz, 8-bit, fully on)
    ledcSetup(0, 5000, 8);
    ledcAttachPin(27, 0);
    ledcWrite(0, 255);

    if (!LittleFS.begin(true))
        Serial.println("[ERROR] LittleFS Failed");

    cache = new DataCache();
    cache->begin();

    dm = new DisplayManager(&tft);
    dm->drawSplash();
    dm->drawBootStatus("Initializing...");

    api = new APIClient(cache);

    dm->drawBootStatus("Connecting to WiFi...");
    if (wifi_init())
    {
        dm->drawBootStatus("Syncing driver standings...");
        api->syncDriversAndStandings();

        dm->drawBootStatus("Syncing constructors...");
        api->syncConstructors();

        dm->drawBootStatus("Syncing calendar...");
        api->syncCalendar();

        cache->driverStandings.shrink_to_fit();
        cache->constructorStandings.shrink_to_fit();
        cache->calendar.shrink_to_fit();
        cache->save();

        dm->drawBootStatus("Setting clock...");
        timeMgr = new TimeManager();
        for (int i = 0; i < 3; i++)
        {
            if (timeMgr->syncNTP())
                break;
            dm->drawBootStatus("NTP retrying...");
            delay(1000);
        }
        if (!timeMgr->isSynced())
            dm->drawBootStatus("Clock sync failed");
    }
    else
    {
        dm->drawBootStatus("Offline mode");
        timeMgr = new TimeManager();
    }

    dm->drawBootStatus("Starting menu...");
    menuView = new MenuView(&tft, dm);
    driverView = new DriverStandingsView(&tft, dm);
    constructorView = new ConstructorStandingsView(&tft, dm);
    calendarView = new CalendarView(&tft, dm);
    newsView = new NewsView(&tft, dm);
    settingsView = new SettingsView(&tft, dm);

    dm->registerView(MenuItem::DRIVER_STANDINGS, driverView);
    dm->registerView(MenuItem::CONSTRUCTOR_STANDINGS, constructorView);
    dm->registerView(MenuItem::CALENDAR, calendarView);
    dm->registerView(MenuItem::NEWS, newsView);
    dm->registerView(MenuItem::SETTINGS, settingsView);

    encoder_init(-100, 100, 0);

    dm->init(menuView);

    Serial.printf("[BOOT] Setup Complete! Heap: %u\n", ESP.getFreeHeap());
}

void loop()
{
    encoder_loop();
    dm->loop();
}
