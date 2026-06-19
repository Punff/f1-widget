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
    bool syncCircuits();
    bool fetchSessionResults(int round, const char *sessionType, std::vector<SessionResult> &results);
    bool fetchCircuitOutline(int sessionKey, int driverNumber, std::vector<CircuitPoint> &points,
                             int maxPoints = 350, const char *dateAfter = nullptr);
    bool fetchNewsFeed();

private:
    bool _cacheOneRound(int round, const RaceMeeting *rm);
    DataCache *_cache;
};
