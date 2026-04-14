#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../data/DataCache.h"

class APIClient {
private:
    HTTPClient http;
    String baseUrl;
    bool makeRequest(const char* endpoint, JsonDocument& doc);

public:
    APIClient();
    bool begin();
    bool fetchStandings(DataCache& cache);
    bool fetchResults(DataCache& cache);
    bool fetchSchedule(DataCache& cache);
    bool fetchAll(DataCache& cache);
};

#endif
