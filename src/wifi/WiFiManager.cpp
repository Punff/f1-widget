#include "WiFiManager.h"
#include <WiFiManager.h> // tzapu library
#include "Arduino.h"

void wifi_init()
{
    WiFiManager wm;

    wm.setConnectTimeout(10);
    wm.setConfigPortalTimeout(180);

    Serial.println("[WiFi] Starting...");

    bool connected = wm.autoConnect("F1Widget");

    if (connected)
    {
        Serial.println("[WiFi] Connected!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("[WiFi] Connection failed, restarting...");
        delay(2000);
        ESP.restart();
    }
}