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
    bool fetchSessionResults(int round, const char *sessionType, std::vector<SessionResult> &results);

private:
    DataCache *_cache;
};