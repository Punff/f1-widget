#include "TimeManager.h"
#include <WiFi.h>

TimeManager::TimeManager() : _utcOffset(2), _baseUtcTime(0), _baseMillis(0) {}

bool TimeManager::syncNTP() {
    Serial.println("[NTP] Starting NTP sync...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    
    // Wait for NTP sync (max 10 seconds)
    time_t now = time(nullptr);
    int retries = 0;
    while ((now < 100000) && (retries < 20)) {
        delay(500);
        now = time(nullptr);
        retries++;
        Serial.printf("[NTP] Waiting... attempt %d, time=%ld\n", retries, now);
    }
    
    if (now < 100000) {
        Serial.println("[NTP] Sync failed after 20 attempts");
        return false;
    }
    
    _baseUtcTime = now;
    _baseMillis = millis();
    Serial.printf("[NTP] Sync successful! Time=%ld\n", now);
    return true;
}

time_t TimeManager::getUTCTime() const {
    if (_baseUtcTime == 0) return time(nullptr);
    return _baseUtcTime + (millis() - _baseMillis) / 1000;
}

time_t TimeManager::getLocalTime() const {
    return getUTCTime() + (_utcOffset * 3600);
}
