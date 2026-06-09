#pragma once
#include <Arduino.h>

// ── WiFi Network File Storage ──────────────────────────────────────────
// Stores up to 8 SSID+password pairs in /wifi_nets.bin for multi-network roaming.
// Replaces boot-time WiFiManager portal with deferred setup from settings.

bool wifi_connect_best();       // Try first saved network, return true if connected
bool wifi_has_saved();          // Any saved networks in file?
int wifi_saved_count();         // Number of saved networks
bool wifi_get_saved(int idx, char *ssid, size_t ssidSize, char *password, size_t pwSize);
bool wifi_save_current();       // Save currently connected WiFi (from WiFi.SSID / .pskKey())
bool wifi_forget(int idx);      // Remove network at index, return true if success
void wifi_start_hotspot();      // Start WiFiManager portal for adding a new network
