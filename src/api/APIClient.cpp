#include "APIClient.h"

// Define static constants
const char APIClient::JOLPI_DRIVERS[] = "https://api.jolpi.ca/ergast/f1/2024/driverstandings/";
const char APIClient::JOLPI_CONSTRUCTORS[] = "https://api.jolpi.ca/ergast/f1/2024/constructorstandings/";
const char APIClient::OPENF1_WEATHER[] = "https://api.openf1.org/v1/weather?session_key=latest";

APIClient::APIClient(DataCache *cache) : _cache(cache) {}

void APIClient::syncPersistentData()
{
    Serial.println("[API] Syncing Standings...");
    fetchDriverStandings();
    fetchConstructorStandings();
}

bool APIClient::fetchDriverStandings()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    HTTPClient http;
    http.begin(JOLPI_DRIVERS);
    http.setTimeout(TIMEOUT_MS);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        JsonDocument doc;
        // STREAMING: No getString() call here. We parse directly from the stream.
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error)
        {
            JsonArray standings = doc["MRData"]["StandingsTable"]["StandingsLists"][0]["DriverStandings"];
            if (!standings.isNull())
            {
                _cache->driverCount = 0;
                for (JsonObject s : standings)
                {
                    if (_cache->driverCount >= DataCache::MAX_DRIVERS)
                        break;
                    DriverStanding &d = _cache->drivers[_cache->driverCount];
                    d.position = s["position"] | 0;
                    d.points = s["points"] | 0;
                    JsonObject driver = s["Driver"];
                    strlcpy(d.lastName, driver["familyName"] | "", sizeof(d.lastName));
                    strlcpy(d.code, driver["code"] | "???", sizeof(d.code));
                    _cache->driverCount++;
                }
                _cache->driversLastUpdated = millis();
                _cache->saveDrivers();
                http.end();
                return true;
            }
        }
        else
        {
            Serial.printf("[API] Parse Error: %s\n", error.c_str());
        }
    }
    http.end();
    return false;
}

bool APIClient::fetchConstructorStandings()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    HTTPClient http;
    http.begin(JOLPI_CONSTRUCTORS);
    http.setTimeout(TIMEOUT_MS);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error)
        {
            JsonArray items = doc["MRData"]["StandingsTable"]["StandingsLists"][0]["ConstructorStandings"];
            if (!items.isNull())
            {
                _cache->constructorCount = 0;
                for (JsonObject item : items)
                {
                    if (_cache->constructorCount >= DataCache::MAX_CONSTRUCTORS)
                        break;
                    ConstructorStanding &c = _cache->constructors[_cache->constructorCount];
                    c.position = item["position"] | 0;
                    c.points = item["points"] | 0;
                    JsonObject construct = item["Constructor"];
                    strlcpy(c.name, construct["name"] | "", sizeof(c.name));
                    _cache->constructorCount++;
                }
                _cache->constructorsLastUpdated = millis();
                _cache->saveConstructors();
                http.end();
                return true;
            }
        }
    }
    http.end();
    return false;
}

bool APIClient::fetchLiveWeather()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;
    HTTPClient http;
    http.begin(OPENF1_WEATHER);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        JsonDocument doc;
        deserializeJson(doc, http.getStream());
        JsonArray arr = doc.as<JsonArray>();
        if (arr.size() > 0)
        {
            JsonObject latest = arr[arr.size() - 1];
            Serial.printf("[Live] Track: %.1f C\n", (float)latest["track_temperature"]);
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}