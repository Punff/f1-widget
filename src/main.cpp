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
#include <esp_wifi.h>
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

    if (!LittleFS.begin(true)) {
        Serial.println("[ERROR] LittleFS Failed");
    }

    SettingsData bootSettings;
    SettingsView::loadSettings(bootSettings);
    ledcWrite(0, bootSettings.brightness);

    cache = new DataCache();
    cache->begin();

    dm = new DisplayManager(&tft);
    dm->setUserBrightness(bootSettings.brightness);
    dm->setDisplayTimeout(bootSettings.displayTimeoutSec);
    dm->drawSplash();

    api = new APIClient(cache);

    // Try WiFi from saved networks (non-blocking with 10s timeout per AP)
    dm->drawBootStatus("Connecting to WiFi...");
    bool wifiConnected = wifi_connect_best();

    if (wifiConnected)
    {
        dm->drawBootStatus("Setting clock...");
        timeMgr = new TimeManager();
        timeMgr->setUTCOffset(bootSettings.utcOffset);
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
        timeMgr->setUTCOffset(bootSettings.utcOffset);
    }

    SettingsView::applyFavTeamColor(bootSettings);

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

    encoder_init(-999999, 999999, 0);

    dm->init(menuView);

    Serial.printf("[BOOT] Setup Complete! Heap: %u\n", ESP.getFreeHeap());
}

void loop()
{
    encoder_loop();
    dm->loop();
}
