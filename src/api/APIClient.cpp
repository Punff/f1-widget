#include "APIClient.h"
#include "../config.h"

APIClient::APIClient() : baseUrl(OPENF1_API_BASE_URL) {}

bool APIClient::begin() {
    return true;
}

bool APIClient::makeRequest(const char* endpoint, JsonDocument& doc) {
    String url = baseUrl + endpoint;
    http.begin(url);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        DeserializationError error = deserializeJson(doc, http.getString());
        http.end();
        return error == DeserializationError::Ok;
    }
    
    http.end();
    return false;
}

bool APIClient::fetchStandings(DataCache& cache) {
    // TODO: Implement standings fetch from OpenF1 API
    return false;
}

bool APIClient::fetchResults(DataCache& cache) {
    // TODO: Implement results fetch from OpenF1 API
    return false;
}

bool APIClient::fetchSchedule(DataCache& cache) {
    // TODO: Implement schedule fetch from OpenF1 API
    return false;
}

bool APIClient::fetchAll(DataCache& cache) {
    bool success = true;
    success &= fetchStandings(cache);
    success &= fetchResults(cache);
    success &= fetchSchedule(cache);
    return success;
}
