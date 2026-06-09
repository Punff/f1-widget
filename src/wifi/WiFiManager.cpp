#include "WiFiManager.h"
#include <WiFiManager.h>
#include "Arduino.h"
#include "../config.h"
#include <LittleFS.h>
#include <WiFi.h>

static const char *WIFI_NET_PATH = "/wifi_nets.bin";
static const uint32_t WIFI_NET_MAGIC = 0x57494649; // "WIFI"

struct WifiNet {
    char ssid[32];
    char password[64];
};

static int _loadNetworks(WifiNet *nets, int maxNets)
{
    if (!LittleFS.exists(WIFI_NET_PATH)) return 0;
    File f = LittleFS.open(WIFI_NET_PATH, FILE_READ);
    if (!f) return 0;
    uint32_t magic;
    if (f.read((uint8_t *)&magic, 4) != 4 || magic != WIFI_NET_MAGIC) {
        f.close(); return 0;
    }
    int count = 0;
    if (f.read((uint8_t *)&count, 4) != 4) { f.close(); return 0; }
    if (count > maxNets) count = maxNets;
    int read = f.read((uint8_t *)nets, count * sizeof(WifiNet));
    f.close();
    if (read != count * (int)sizeof(WifiNet)) return 0;
    return count;
}

static void _saveNetworks(const WifiNet *nets, int count)
{
    File f = LittleFS.open(WIFI_NET_PATH, FILE_WRITE);
    if (!f) return;
    f.write((uint8_t *)&WIFI_NET_MAGIC, 4);
    f.write((uint8_t *)&count, 4);
    if (count > 0) f.write((uint8_t *)nets, count * sizeof(WifiNet));
    f.close();
}

bool wifi_has_saved()
{
    return wifi_saved_count() > 0;
}

int wifi_saved_count()
{
    WifiNet nets[8];
    return _loadNetworks(nets, 8);
}

bool wifi_get_saved(int idx, char *ssid, size_t ssidSize, char *password, size_t pwSize)
{
    WifiNet nets[8];
    int count = _loadNetworks(nets, 8);
    if (idx < 0 || idx >= count) return false;
    strlcpy(ssid, nets[idx].ssid, ssidSize);
    strlcpy(password, nets[idx].password, pwSize);
    return true;
}

bool wifi_save_current()
{
    String curSsid = WiFi.SSID();
    if (curSsid.length() == 0) return false;

    WifiNet nets[8];
    int count = _loadNetworks(nets, 8);

    // Replace if already exists, otherwise append
    for (int i = 0; i < count; i++) {
        if (strcmp(nets[i].ssid, curSsid.c_str()) == 0) {
            strlcpy(nets[i].password, WiFi.psk().c_str(), sizeof(nets[i].password));
            _saveNetworks(nets, count);
            return true;
        }
    }

    if (count >= 8) return false; // Max 8 saved networks
    strlcpy(nets[count].ssid, curSsid.c_str(), sizeof(nets[count].ssid));
    strlcpy(nets[count].password, WiFi.psk().c_str(), sizeof(nets[count].password));
    _saveNetworks(nets, count + 1);
    return true;
}

bool wifi_forget(int idx)
{
    WifiNet nets[8];
    int count = _loadNetworks(nets, 8);
    if (idx < 0 || idx >= count) return false;

    // Shuffle remaining entries down
    for (int i = idx; i < count - 1; i++)
        nets[i] = nets[i + 1];
    _saveNetworks(nets, count - 1);
    return true;
}

bool wifi_connect_best()
{
    WifiNet nets[8];
    int count = _loadNetworks(nets, 8);
    if (count == 0) return false;

    WiFi.mode(WIFI_STA);

    for (int i = 0; i < count; i++)
    {
        Serial.printf("[WiFi] Trying: %s\n", nets[i].ssid);
        WiFi.begin(nets[i].ssid, nets[i].password);

        unsigned long start = millis();
        int dots = 0;
        while (millis() - start < 10000)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.printf("\n[WiFi] Connected to %s\n", nets[i].ssid);
                Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
                return true;
            }
            delay(500);
            dots++;
            if (dots % 4 == 0) Serial.print(".");
        }
        Serial.printf("\n[WiFi] %s: timeout\n", nets[i].ssid);
        WiFi.disconnect(false, true); // Erase NVS for this attempt only
    }

    Serial.println("[WiFi] No saved network could be connected");
    return false;
}

void wifi_start_hotspot()
{
    Serial.println("[WiFi] Starting hotspot portal...");
    WiFiManager wm;
    wm.setCustomHeadElement("<style>body{background-color:#e10600;color:white;} .btn{background-color:#1f1f1f;}</style>");
    wm.setConnectTimeout(20);
    wm.setConfigPortalTimeout(180);
    bool connected = wm.autoConnect(WIFI_AP_NAME);
    if (connected) wifi_save_current();
}
