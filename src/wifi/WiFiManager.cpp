#include "WiFiManager.h"
#include <WiFiManager.h> // The actual library
#include "Arduino.h"

bool wifi_init()
{
    WiFiManager wm;

    // Optional: Customize the UI of the config portal to match F1
    wm.setCustomHeadElement("<style>body{background-color:#e10600;color:white;} .btn{background-color:#1f1f1f;}</style>");

    wm.setConnectTimeout(20);       // How long to try connecting to saved WiFi
    wm.setConfigPortalTimeout(180); // How long to keep the "F1Widget" hotspot alive

    Serial.println("[WiFi] Attempting to connect...");

    // This will either connect to saved WiFi or start the "F1Widget" AP
    bool connected = wm.autoConnect("F1Widget");

    if (connected)
    {
        Serial.println("[WiFi] Connected successfully!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("[WiFi] Connection failed or timed out.");
    }

    return connected;
}