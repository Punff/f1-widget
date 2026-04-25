#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../data/DataCache.h"

class APIClient
{
public:
    APIClient(DataCache *cache);

    // Orchestrator: Fetches standings (Ergast) and basic race info
    void syncPersistentData();

    // Live Fetchers (OpenF1)
    bool fetchLiveWeather();
    bool fetchDriverTelemetry(int driverNumber);

    // Standings Fetchers (Ergast/Jolpi)
    bool fetchDriverStandings();
    bool fetchConstructorStandings();

private:
    DataCache *_cache;

    // Internal Helpers
    String makeRequest(const char *url);
    bool _parseDriverStandings(const String &payload);
    bool _parseConstructorStandings(const String &payload);

    // Endpoints
    static const char JOLPI_DRIVERS[];
    static const char JOLPI_CONSTRUCTORS[];
    static const char OPENF1_WEATHER[];

    // Configuration
    static constexpr int TIMEOUT_MS = 8000;
};

#endif