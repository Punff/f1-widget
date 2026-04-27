#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../data/DataCache.h"

class APIClient
{
public:
    APIClient(DataCache *cache);

    // Primary Orchestrator
    bool syncAll();

    // Specific Syncers
    bool syncDriversAndStandings(); // Merges OpenF1 + Jolpica
    bool syncConstructors();        // Jolpica
    bool syncCalendar();            // OpenF1

private:
    DataCache *_cache;
    String _httpGet(String url);

    // Helper to find a driver in the cache by name or code to merge points
    DriverStanding *_findDriver(const char *acronym);
};