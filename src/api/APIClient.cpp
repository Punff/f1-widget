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
    Serial.println("[CALENDAR] Fetching from Jolpica...");

    // 1. Jolpica for season calendar (rounds, dates, countries)
    String url = "https://api.jolpi.ca/ergast/f1/2026.json";
    String payload = _httpGet(url);

    if (payload.length() == 0)
    {
        Serial.println("[CALENDAR] ERROR: Empty response from Jolpica");
        return false;
    }

    Serial.printf("[CALENDAR] Got %d bytes from Jolpica\n", payload.length());

    JsonDocument doc;
    if (deserializeJson(doc, payload))
    {
        Serial.println("[CALENDAR] ERROR: JSON parse failed");
        return false;
    }

    auto races = doc["MRData"]["RaceTable"]["Races"].as<JsonArray>();
    if (races.size() == 0)
    {
        Serial.println("[CALENDAR] ERROR: No races found in JSON");
        return false;
    }

    Serial.printf("[CALENDAR] Found %d races\n", races.size());

    // Only clear calendar after successful parse
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
        rm.dateStart[0] = '\0';
        rm.sessionCount = 0;
        memset(rm.sessions, 0, sizeof(rm.sessions));
        _cache->calendar.push_back(rm);
    }

    Serial.printf("[CALENDAR] Populated %d races\n", _cache->calendar.size());

    // 2. OpenF1 meetings to get meetingKey per race (optional, don't fail if this errors)
    Serial.println("[CALENDAR] Fetching OpenF1 meetings...");
    String openF1MeetingsUrl = "https://api.openf1.org/v1/meetings?year=2026";
    String openF1Payload = _httpGet(openF1MeetingsUrl);
    JsonDocument openF1Doc;
    if (!deserializeJson(openF1Doc, openF1Payload))
    {
        Serial.printf("[CALENDAR] Got %d meetings from OpenF1\n", openF1Doc.as<JsonArray>().size());
        for (JsonObject meeting : openF1Doc.as<JsonArray>())
        {
            int meetingKey = meeting["meeting_key"];
            const char *circuitShortName = meeting["circuit_short_name"];
            for (auto &rm : _cache->calendar)
            {
                if (strcmp(rm.circuit.shortName, circuitShortName) == 0)
                {
                    rm.meetingKey = meetingKey;
                    break;
                }
            }
        }
    }
    else
    {
        Serial.println("[CALENDAR] WARNING: Failed to fetch OpenF1 meetings (non-critical)");
    }

    // 3. Fetch sessions for each meeting from OpenF1 (optional)
    Serial.println("[CALENDAR] Fetching sessions from OpenF1...");
    int sessionsFetched = 0;
    for (auto &rm : _cache->calendar)
    {
        if (rm.meetingKey == 0)
            continue;
        String sessionsUrl = "https://api.openf1.org/v1/sessions?meeting_key=" + String(rm.meetingKey);
        String sessionsPayload = _httpGet(sessionsUrl);
        JsonDocument sessionsDoc;
        if (deserializeJson(sessionsDoc, sessionsPayload))
            continue;
        rm.sessionCount = 0;
        for (JsonObject session : sessionsDoc.as<JsonArray>())
        {
            if (rm.sessionCount >= 6)
                break;
            strlcpy(rm.sessions[rm.sessionCount].name, session["session_name"], 32);
            strlcpy(rm.sessions[rm.sessionCount].dateUtc, session["date_start"], 32);
            rm.sessionCount++;
            sessionsFetched++;
        }
    }

    Serial.printf("[CALENDAR] Fetched %d sessions total\n", sessionsFetched);
    Serial.println("[CALENDAR] Sync complete!");
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
