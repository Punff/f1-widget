#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../data/DataCache.h"

class APIClient
{
public:
    APIClient(DataCache *cache);

    bool syncAll();
    bool syncDriversAndStandings();
    bool syncConstructors();
    bool syncCalendar();

private:
    DataCache *_cache;
};