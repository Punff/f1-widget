#include "APIClient.h"
#include <algorithm>
#include <WiFi.h>

// FIX: Ensure the constructor is defined here
APIClient::APIClient(DataCache *cache) : _cache(cache) {}

// FIX: Ensure the orchestrator method is defined here
bool APIClient::syncAll()
{
    bool ok = true;
    ok &= syncDriversAndStandings();
    ok &= syncConstructors();
    ok &= syncCalendar();

    // Always save - if some syncs fail, we still save what we got
    _cache->driverStandings.shrink_to_fit();
    _cache->constructorStandings.shrink_to_fit();
    _cache->calendar.shrink_to_fit();
    _cache->save();

    if (ok)
    {
        Serial.println("[SYNC] All syncs successful!");
    }
    else
    {
        Serial.println("[SYNC] Some syncs failed, but data saved where possible");
    }
    return ok;
}

bool APIClient::syncCalendar()
{
    Serial.println("[CALENDAR] Fetching from Jolpica...");

    // 1. Jolpica for season calendar
    {
        HTTPClient http;
        http.begin("https://api.jolpi.ca/ergast/f1/2026.json");
        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) return false;

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, http.getStream());
        if (error) {
            Serial.printf("[CALENDAR] JSON Error: %s\n", error.c_str());
            return false;
        }

        auto races = doc["MRData"]["RaceTable"]["Races"].as<JsonArray>();
        if (races.size() == 0) return false;

        _cache->calendar.clear();
        for (JsonObject r : races)
        {
            RaceMeeting rm;
            rm.round = r["round"].as<int>();
            strlcpy(rm.date, r["date"], 12);
            strlcpy(rm.officialName, r["raceName"], 128);
            strlcpy(rm.circuit.shortName, r["Circuit"]["circuitName"], 64);
            strlcpy(rm.circuit.location, r["Circuit"]["Location"]["locality"], 64);
            strlcpy(rm.circuit.countryName, r["Circuit"]["Location"]["country"], 32);
            rm.meetingKey = 0;
            rm.sessionCount = 0;
            memset(rm.sessions, 0, sizeof(rm.sessions));
            _cache->calendar.push_back(rm);
        }
        Serial.printf("[CALENDAR] Populated %d races\n", _cache->calendar.size());
    }

    // 2. OpenF1 meetings in bulk
    {
        Serial.println("[CALENDAR] Fetching OpenF1 meetings...");
        HTTPClient http;
        http.begin("https://api.openf1.org/v1/meetings?year=2026");
        if (http.GET() == HTTP_CODE_OK)
        {
            JsonDocument doc;
            if (!deserializeJson(doc, http.getStream()))
            {
                for (JsonObject meeting : doc.as<JsonArray>())
                {
                    int mKey = meeting["meeting_key"];
                    const char *cName = meeting["circuit_short_name"];
                    for (auto &rm : _cache->calendar)
                    {
                        if (strcmp(rm.circuit.shortName, cName) == 0) {
                            rm.meetingKey = mKey;
                            break;
                        }
                    }
                }
            }
        }
    }

    // 3. Fetch ALL sessions for the year in ONE call
    {
        Serial.println("[CALENDAR] Fetching ALL sessions bulk...");
        HTTPClient http;
        http.begin("https://api.openf1.org/v1/sessions?year=2026");
        if (http.GET() == HTTP_CODE_OK)
        {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, http.getStream());
            if (!error)
            {
                int sessionCount = 0;
                for (JsonObject s : doc.as<JsonArray>())
                {
                    int mKey = s["meeting_key"];
                    for (auto &rm : _cache->calendar)
                    {
                        if (rm.meetingKey == mKey && rm.sessionCount < 6)
                        {
                            strlcpy(rm.sessions[rm.sessionCount].name, s["session_name"], 32);
                            strlcpy(rm.sessions[rm.sessionCount].dateUtc, s["date_start"], 32);
                            rm.sessionCount++;
                            sessionCount++;
                            break;
                        }
                    }
                }
                Serial.printf("[CALENDAR] Fetched %d sessions bulk\n", sessionCount);
            }
        }
    }

    Serial.println("[CALENDAR] Sync complete!");
    return true;
}

bool APIClient::syncDriversAndStandings()
{
    // Drivers from OpenF1
    {
        HTTPClient http;
        http.begin("https://api.openf1.org/v1/drivers?session_key=latest");
        if (http.GET() != HTTP_CODE_OK) return false;

        JsonDocument doc;
        if (deserializeJson(doc, http.getStream())) return false;

        _cache->driverStandings.clear();
        for (JsonObject d : doc.as<JsonArray>())
        {
            DriverStanding ds;
            ds.driver.number = d["driver_number"];
            strlcpy(ds.driver.firstName, d["first_name"], 32);
            strlcpy(ds.driver.lastName, d["last_name"], 32);
            strlcpy(ds.driver.fullName, d["full_name"], 64);
            strlcpy(ds.driver.acronym, d["name_acronym"], 4);
            strlcpy(ds.driver.team.name, d["team_name"], 64);
            ds.driver.team.teamColor = DataCache::hexTo565(d["team_colour"]);
            ds.position = 0;
            ds.points = 0;
            _cache->driverStandings.push_back(ds);
        }
    }

    // Standings from Jolpica
    {
        HTTPClient http;
        http.begin("https://api.jolpi.ca/ergast/f1/2026/driverstandings.json");
        if (http.GET() == HTTP_CODE_OK)
        {
            JsonDocument doc;
            if (!deserializeJson(doc, http.getStream()))
            {
                JsonArray standings = doc["MRData"]["StandingsTable"]["StandingsLists"][0]["DriverStandings"];
                for (JsonObject s : standings)
                {
                    const char *code = s["Driver"]["code"];
                    for (auto &ds : _cache->driverStandings)
                    {
                        if (strcmp(ds.driver.acronym, code) == 0)
                        {
                            ds.points = s["points"].as<int>();
                            break;
                        }
                    }
                }
            }
        }
    }

    std::sort(_cache->driverStandings.begin(), _cache->driverStandings.end(),
              [](const DriverStanding &a, const DriverStanding &b) { return a.points > b.points; });

    for (size_t i = 0; i < _cache->driverStandings.size(); i++)
        _cache->driverStandings[i].position = (int)(i + 1);

    return true;
}

bool APIClient::syncConstructors()
{
    HTTPClient http;
    http.begin("https://api.jolpi.ca/ergast/f1/2026/constructorstandings.json");
    if (http.GET() != HTTP_CODE_OK) return false;

    JsonDocument doc;
    if (deserializeJson(doc, http.getStream())) return false;

    auto standings = doc["MRData"]["StandingsTable"]["StandingsLists"][0]["ConstructorStandings"].as<JsonArray>();

    _cache->constructorStandings.clear();
    for (JsonObject s : standings)
    {
        ConstructorStanding cs;
        cs.position = s["position"].as<int>();
        cs.points = s["points"].as<int>();
        strlcpy(cs.team.name, s["Constructor"]["name"], 64);
        strlcpy(cs.team.id, s["Constructor"]["constructorId"], 32);
        cs.team.teamColor = 0x4208;
        for (const auto &ds : _cache->driverStandings)
        {
            if (strstr(ds.driver.team.name, cs.team.name) != nullptr) {
                cs.team.teamColor = ds.driver.team.teamColor;
                break;
            }
        }
        _cache->constructorStandings.push_back(cs);
    }
    return true;
}

String APIClient::_httpGet(String url)
{
    // Keeping for compatibility but favoring direct sync methods
    HTTPClient http;
    http.begin(url);
    if (http.GET() == HTTP_CODE_OK) return http.getString();
    return "";
}
