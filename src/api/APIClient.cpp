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
    ok &= syncConstructors(); // Uncomment when ready
    ok &= syncCalendar();     // Uncomment when ready

    if (ok)
    {
        _cache->save();
    }
    return ok;
}

bool APIClient::syncDriversAndStandings()
{
    String openf1Url = "https://api.openf1.org/v1/drivers?session_key=latest";
    String payload = _httpGet(openf1Url);

    JsonDocument openDoc;
    if (deserializeJson(openDoc, payload))
        return false;

    _cache->driverStandings.clear();
    for (JsonObject d : openDoc.as<JsonArray>())
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

    String jolpicaUrl = "https://api.jolpi.ca/ergast/f1/2026/driverstandings.json";
    payload = _httpGet(jolpicaUrl);

    JsonDocument jolpDoc;
    deserializeJson(jolpDoc, payload);

    JsonArray standings = jolpDoc["MRData"]["StandingsTable"]["StandingsLists"][0]["DriverStandings"];
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

    // ── SORTING ──
    std::sort(_cache->driverStandings.begin(), _cache->driverStandings.end(),
              [](const DriverStanding &a, const DriverStanding &b)
              {
                  return a.points > b.points;
              });

    // ── POSITIONING ──
    for (size_t i = 0; i < _cache->driverStandings.size(); i++)
    {
        _cache->driverStandings[i].position = (int)(i + 1);
    }

    return true;
}

bool APIClient::syncConstructors()
{
    String url = "https://api.jolpi.ca/ergast/f1/2026/constructorstandings.json";
    String payload = _httpGet(url);

    JsonDocument doc;
    if (deserializeJson(doc, payload))
        return false;

    auto standings = doc["MRData"]["StandingsTable"]["StandingsLists"][0]["ConstructorStandings"].as<JsonArray>();

    _cache->constructorStandings.clear();
    for (JsonObject s : standings)
    {
        ConstructorStanding cs;
        cs.position = s["position"].as<int>();
        cs.points = s["points"].as<int>();
        strlcpy(cs.team.name, s["Constructor"]["name"], 64);
        strlcpy(cs.team.id, s["Constructor"]["constructorId"], 32);

        // Match color from driver data
        cs.team.teamColor = 0x4208;
        for (const auto &ds : _cache->driverStandings)
        {
            if (strstr(ds.driver.team.name, cs.team.name) != nullptr)
            {
                cs.team.teamColor = ds.driver.team.teamColor;
                break;
            }
        }
        _cache->constructorStandings.push_back(cs);
    }
    return true;
}

bool APIClient::syncCalendar()
{
    // Jolpica/Ergast is the reliable source for the full season schedule
    String url = "https://api.jolpi.ca/ergast/f1/2026.json";
    String payload = _httpGet(url);

    JsonDocument doc;
    if (deserializeJson(doc, payload))
        return false;

    auto races = doc["MRData"]["RaceTable"]["Races"].as<JsonArray>();

    _cache->calendar.clear();
    for (JsonObject r : races)
    {
        RaceMeeting rm;

        // Populate the new members we just added
        rm.round = r["round"].as<int>();
        strlcpy(rm.date, r["date"], 12); // Format: 2026-03-15

        // Populate standard info
        strlcpy(rm.officialName, r["raceName"], 128);

        // Populate nested circuit info
        strlcpy(rm.circuit.shortName, r["Circuit"]["circuitName"], 64);
        strlcpy(rm.circuit.location, r["Circuit"]["Location"]["locality"], 64);
        strlcpy(rm.circuit.countryName, r["Circuit"]["Location"]["country"], 32);

        // These can remain 0/empty as Jolpica doesn't provide them
        rm.meetingKey = 0;
        rm.dateStart[0] = '\0';

        _cache->calendar.push_back(rm);
    }
    return true;
}
String APIClient::_httpGet(String url)
{
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
        return http.getString();
    return "";
}