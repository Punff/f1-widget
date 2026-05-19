#include "APIClient.h"
#include <algorithm>
#include <WiFi.h>

APIClient::APIClient(DataCache *cache) : _cache(cache) {}

bool APIClient::syncAll()
{
    bool ok = true;
    ok &= syncDriversAndStandings();
    ok &= syncConstructors();
    ok &= syncCalendar();

    _cache->driverStandings.shrink_to_fit();
    _cache->constructorStandings.shrink_to_fit();
    _cache->calendar.shrink_to_fit();
    _cache->save();

    if (ok)
        Serial.println("[SYNC] All syncs successful!");
    else
        Serial.println("[SYNC] Some syncs failed, but data saved where possible");
    return ok;
}

bool APIClient::syncDriversAndStandings()
{
    String season = String(_cache->currentSeason);
    Serial.printf("[SYNC] Drivers for %s...\n", season.c_str());

    // 1. Drivers from OpenF1 — fetch body first, close HTTP, THEN parse
    String driverJson;
    {
        HTTPClient http;
        http.begin("https://api.openf1.org/v1/drivers?session_key=latest");
        if (http.GET() == HTTP_CODE_OK)
            driverJson = http.getString();
    }

    if (driverJson.length() > 0)
    {
        JsonDocument doc;
        if (!deserializeJson(doc, driverJson) && doc.is<JsonArray>())
        {
            _cache->driverStandings.clear();
            _cache->driverStandings.reserve(doc.as<JsonArray>().size());
            for (JsonObject d : doc.as<JsonArray>())
            {
                DriverStanding ds;
                ds.driver.number = d["driver_number"];
                strlcpy(ds.driver.firstName, d["first_name"] | "", 32);
                strlcpy(ds.driver.lastName, d["last_name"] | "", 32);
                strlcpy(ds.driver.fullName, d["full_name"] | "", 64);
                strlcpy(ds.driver.broadcastName, d["broadcast_name"] | "", 32);
                strlcpy(ds.driver.acronym, d["name_acronym"] | "", 4);
                strlcpy(ds.driver.team.name, d["team_name"] | "", 64);
                ds.driver.team.teamColor = DataCache::hexTo565(d["team_colour"]);
                ds.position = 0;
                ds.points = 0;
                ds.wins = 0;
                _cache->driverStandings.push_back(ds);
            }
            Serial.printf("[SYNC] %zu drivers from OpenF1\n", _cache->driverStandings.size());
        }
    }

    // 2. Standings from Jolpica — fetch body first, close HTTP, THEN parse
    String standingJson;
    {
        HTTPClient http;
        http.begin("https://api.jolpi.ca/ergast/f1/" + season + "/driverstandings.json");
        if (http.GET() == HTTP_CODE_OK)
            standingJson = http.getString();
    }

    if (standingJson.length() > 0)
    {
        JsonDocument doc;
        if (!deserializeJson(doc, standingJson))
        {
            JsonArray lists = doc["MRData"]["StandingsTable"]["StandingsLists"];
            if (lists.size() > 0)
            {
                int matched = 0;
                JsonArray dsArr = lists[0]["DriverStandings"].as<JsonArray>();
                for (JsonObject s : dsArr)
                {
                    const char *code = s["Driver"]["code"];
                    if (!code) continue;
                    int pts = s["points"].as<int>();
                    for (auto &ds : _cache->driverStandings)
                    {
                        if (strcmp(ds.driver.acronym, code) == 0)
                        {
                            ds.points = pts;
                            matched++;
                            break;
                        }
                    }
                }
                Serial.printf("[SYNC] Matched %d drivers by points\n", matched);
            }
        }
    }

    // Sort by points descending and assign positions
    std::sort(_cache->driverStandings.begin(), _cache->driverStandings.end(),
              [](const DriverStanding &a, const DriverStanding &b) { return a.points > b.points; });

    for (size_t i = 0; i < _cache->driverStandings.size(); i++)
        _cache->driverStandings[i].position = (int)(i + 1);

    return true;
}

bool APIClient::syncConstructors()
{
    String season = String(_cache->currentSeason);
    Serial.printf("[SYNC] Constructors for %s...\n", season.c_str());

    String json;
    {
        HTTPClient http;
        http.begin("https://api.jolpi.ca/ergast/f1/" + season + "/constructorstandings.json");
        if (http.GET() != HTTP_CODE_OK) return false;
        json = http.getString();
    }

    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;

    JsonArray lists = doc["MRData"]["StandingsTable"]["StandingsLists"];
    if (lists.size() == 0) return false;

    auto standings = lists[0]["ConstructorStandings"].as<JsonArray>();
    _cache->constructorStandings.clear();
    _cache->constructorStandings.reserve(standings.size());
    for (JsonObject s : standings)
    {
        ConstructorStanding cs;
        cs.position = s["position"].as<int>();
        cs.points = s["points"].as<int>();
        strlcpy(cs.team.name, s["Constructor"]["name"] | "", 64);
        strlcpy(cs.team.id, s["Constructor"]["constructorId"] | "", 32);

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
    String season = String(_cache->currentSeason);
    Serial.printf("[SYNC] Calendar for %s...\n", season.c_str());

    String raceJson;
    {
        HTTPClient http;
        http.begin("https://api.jolpi.ca/ergast/f1/" + season + ".json");
        if (http.GET() != HTTP_CODE_OK) return false;
        raceJson = http.getString();
    }

    JsonDocument doc;
    if (deserializeJson(doc, raceJson)) return false;

    auto races = doc["MRData"]["RaceTable"]["Races"].as<JsonArray>();
    if (races.size() == 0) return false;

    _cache->calendar.clear();
    _cache->calendar.reserve(races.size());

    const char* sKeys[] = {"FirstPractice","SecondPractice","ThirdPractice","SprintQualifying","Sprint","Qualifying"};
    const char* sNames[] = {"Practice 1","Practice 2","Practice 3","Sprint Qualifying","Sprint","Qualifying"};

    for (JsonObject r : races)
    {
        RaceMeeting rm;
        rm.round = r["round"].as<int>();
        strlcpy(rm.date, r["date"] | "", 12);
        strlcpy(rm.officialName, r["raceName"] | "", 128);
        strlcpy(rm.circuit.shortName, r["Circuit"]["circuitName"] | "", 64);
        strlcpy(rm.circuit.location, r["Circuit"]["Location"]["locality"] | "", 64);
        strlcpy(rm.circuit.countryName, r["Circuit"]["Location"]["country"] | "", 32);
        rm.meetingKey = 0;

        // Parse embedded sessions from Jolpica
        rm.sessionCount = 0;
        memset(rm.sessions, 0, sizeof(rm.sessions));
        for (int si = 0; si < 6 && rm.sessionCount < 6; si++)
        {
            JsonVariant sess = r[sKeys[si]];
            if (!sess.isNull())
            {
                strlcpy(rm.sessions[rm.sessionCount].name, sNames[si], 32);
                snprintf(rm.sessions[rm.sessionCount].dateUtc, 32, "%sT%s",
                    sess["date"] | "",
                    sess["time"] | "");
                rm.sessionCount++;
            }
        }
        // Always add Race from top-level date/time
        if (rm.sessionCount < 6)
        {
            strlcpy(rm.sessions[rm.sessionCount].name, "Race", 32);
            snprintf(rm.sessions[rm.sessionCount].dateUtc, 32, "%sT%s",
                r["date"] | "",
                r["time"] | "");
            rm.sessionCount++;
        }

        _cache->calendar.push_back(rm);
    }

    int totalSessions = 0;
    for (auto &rm : _cache->calendar) totalSessions += rm.sessionCount;
    Serial.printf("[SYNC] Calendar complete. %d races, %d sessions.\n", _cache->calendar.size(), totalSessions);
    return true;
}


