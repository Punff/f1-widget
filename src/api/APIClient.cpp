#include "APIClient.h"
#include "../display/views/UI.h"
#include "../config.h"
#include <algorithm>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>

APIClient::APIClient(DataCache *cache) : _cache(cache) {}

// Convert "VERSTAPPEN" → "Verstappen", "DE VRIES" → "De Vries"
static void properCase(char *out, size_t outSize, const char *in)
{
    if (!in || !in[0]) { out[0] = 0; return; }
    bool capNext = true;
    size_t wi = 0;
    for (const char *p = in; *p && wi < outSize - 1; p++) {
        if (*p == ' ') {
            out[wi++] = ' ';
            capNext = true;
        } else if (capNext) {
            out[wi++] = toupper(*p);
            capNext = false;
        } else {
            out[wi++] = tolower(*p);
        }
    }
    out[wi] = 0;
}

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

    // Background cache circuits for all completed rounds
    syncCircuits();

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

    // 1. Standings from Jolpica — full list of championship drivers (source of truth)
    String standingJson;
    {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, String(JOLPICA_API_BASE) + "/" + season + "/driverstandings.json");
        http.setTimeout(10000);
        if (http.GET() == HTTP_CODE_OK)
            standingJson = http.getString();
    }
    bool hasStandings = standingJson.length() > 0;

    // 2. Drivers from OpenF1 — supplementary data (team colors, broadcast names)
    String driverJson;
    {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, String(OPENF1_API_BASE) + "/drivers?session_key=latest");
        http.setTimeout(10000);
        if (http.GET() == HTTP_CODE_OK)
            driverJson = http.getString();
    }
    bool hasDrivers = driverJson.length() > 0;

    if (!hasStandings && !hasDrivers) return false;

    // Parse OpenF1 drivers into temp storage for lookup by acronym
    struct OpenF1Info {
        char acronym[4];
        char firstName[16];
        char lastName[24];
        char fullName[48];
        char broadcastName[24];
        char teamName[64];
        char countryCode[4];
        uint16_t teamColor;
        int number;
    };
    std::vector<OpenF1Info> openF1Drivers;
    if (hasDrivers) {
        JsonDocument doc;
        if (!deserializeJson(doc, driverJson) && doc.is<JsonArray>()) {
            openF1Drivers.reserve(doc.as<JsonArray>().size());
            for (JsonObject d : doc.as<JsonArray>()) {
                OpenF1Info di;
                strlcpy(di.acronym, d["name_acronym"] | "", sizeof(di.acronym));
                strlcpy(di.firstName, d["first_name"] | "", sizeof(di.firstName));
                strlcpy(di.lastName, d["last_name"] | "", sizeof(di.lastName));
                strlcpy(di.fullName, d["full_name"] | "", sizeof(di.fullName));
                strlcpy(di.broadcastName, d["broadcast_name"] | "", sizeof(di.broadcastName));
                strlcpy(di.teamName, d["team_name"] | "", sizeof(di.teamName));
                strlcpy(di.countryCode, d["country_code"] | "", sizeof(di.countryCode));
                di.teamColor = DataCache::hexTo565(d["team_colour"]);
                di.number = d["driver_number"];
                openF1Drivers.push_back(di);
            }
        }
    }

    auto findOpenF1 = [&](const char *code) -> OpenF1Info* {
        for (auto &di : openF1Drivers)
            if (strcmp(di.acronym, code) == 0) return &di;
        return nullptr;
    };

    _cache->driverStandings.clear();
    _cache->driverStandings.reserve(hasStandings ? 24 : openF1Drivers.size());

    if (hasStandings)
    {
        // Build from Jolpica (all championship drivers), merge OpenF1 data
        JsonDocument doc;
        if (!deserializeJson(doc, standingJson))
        {
            JsonArray lists = doc["MRData"]["StandingsTable"]["StandingsLists"];
            if (lists.size() > 0)
            {
                JsonArray dsArr = lists[0]["DriverStandings"].as<JsonArray>();
                for (JsonObject s : dsArr)
                {
                    DriverStanding ds;
                    ds.position = s["position"].as<int>();
                    ds.points = s["points"].as<float>();
                    ds.wins = s["wins"].as<int>();
                    ds.driver.number = s["Driver"]["permanentNumber"].as<int>();

                    const char *code = s["Driver"]["code"] | "?";
                    Serial.printf("[SYNC] %s: Pts=%.1f Wins=%d Pos=%d\n", code, ds.points, ds.wins, ds.position);

                    strlcpy(ds.driver.acronym, s["Driver"]["code"] | "", sizeof(ds.driver.acronym));
                    strlcpy(ds.driver.firstName, s["Driver"]["givenName"] | "", sizeof(ds.driver.firstName));
                    {
                        const char *rawLast = s["Driver"]["familyName"] | "";
                        char properLast[24];
                        properCase(properLast, sizeof(properLast), rawLast);
                        strlcpy(ds.driver.lastName, properLast, sizeof(ds.driver.lastName));
                        String full = String(s["Driver"]["givenName"] | "") + " " + String(properLast);
                        strlcpy(ds.driver.fullName, full.c_str(), sizeof(ds.driver.fullName));
                    }
                    strlcpy(ds.driver.broadcastName, s["Driver"]["code"] | "", sizeof(ds.driver.broadcastName));
                    strlcpy(ds.driver.nationality, s["Driver"]["nationality"] | "", sizeof(ds.driver.nationality));
                    strlcpy(ds.driver.dateOfBirth, s["Driver"]["dateOfBirth"] | "", sizeof(ds.driver.dateOfBirth));

                    // Constructor info
                    JsonArray constructors = s["Constructors"].as<JsonArray>();
                    if (constructors.size() > 0) {
                        strlcpy(ds.driver.team.name, constructors[0]["name"] | "", sizeof(ds.driver.team.name));
                        strlcpy(ds.driver.team.id, constructors[0]["constructorId"] | "", sizeof(ds.driver.team.id));
                    }

                    // Merge OpenF1 data (richer names, team colors, country code)
                    OpenF1Info *ofi = findOpenF1(ds.driver.acronym);
                    if (ofi) {
                        strlcpy(ds.driver.firstName, ofi->firstName, sizeof(ds.driver.firstName));
                        strlcpy(ds.driver.lastName, ofi->lastName, sizeof(ds.driver.lastName));
                        strlcpy(ds.driver.fullName, ofi->fullName, sizeof(ds.driver.fullName));
                        strlcpy(ds.driver.broadcastName, ofi->broadcastName, sizeof(ds.driver.broadcastName));
                        strlcpy(ds.driver.team.name, ofi->teamName, sizeof(ds.driver.team.name));
                        if (ofi->countryCode[0]) strlcpy(ds.driver.nationality, ofi->countryCode, sizeof(ds.driver.nationality));
                        ds.driver.team.teamColor = ofi->teamColor;
                        ds.driver.number = ofi->number;
                    } else {
                        ds.driver.team.teamColor = UI::COL_TEAM_GREY;
                    }

                    _cache->driverStandings.push_back(ds);
                }
            }
        }

        // Append any OpenF1 drivers not in Jolpica standings (reserves/test drivers)
        for (auto &ofi : openF1Drivers) {
            bool found = false;
            for (auto &ds : _cache->driverStandings) {
                if (strcmp(ds.driver.acronym, ofi.acronym) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                DriverStanding ds;
                ds.position = 0;
                ds.points = 0;
                ds.wins = 0;
                ds.driver.number = ofi.number;
                strlcpy(ds.driver.acronym, ofi.acronym, sizeof(ds.driver.acronym));
                strlcpy(ds.driver.firstName, ofi.firstName, sizeof(ds.driver.firstName));
                strlcpy(ds.driver.lastName, ofi.lastName, sizeof(ds.driver.lastName));
                strlcpy(ds.driver.fullName, ofi.fullName, sizeof(ds.driver.fullName));
                strlcpy(ds.driver.broadcastName, ofi.broadcastName, sizeof(ds.driver.broadcastName));
                strlcpy(ds.driver.team.name, ofi.teamName, sizeof(ds.driver.team.name));
                ds.driver.team.teamColor = ofi.teamColor;
                _cache->driverStandings.push_back(ds);
            }
        }
    }
    else
    {
        // Fallback: use only OpenF1 drivers (no Jolpica available)
        for (auto &ofi : openF1Drivers) {
            DriverStanding ds;
            ds.position = 0;
            ds.points = 0;
            ds.wins = 0;
            ds.driver = {};
            ds.driver.number = ofi.number;
            strlcpy(ds.driver.acronym, ofi.acronym, sizeof(ds.driver.acronym));
            strlcpy(ds.driver.firstName, ofi.firstName, sizeof(ds.driver.firstName));
            strlcpy(ds.driver.lastName, ofi.lastName, sizeof(ds.driver.lastName));
            strlcpy(ds.driver.fullName, ofi.fullName, sizeof(ds.driver.fullName));
            strlcpy(ds.driver.broadcastName, ofi.broadcastName, sizeof(ds.driver.broadcastName));
            strlcpy(ds.driver.team.name, ofi.teamName, sizeof(ds.driver.team.name));
            ds.driver.team.teamColor = ofi.teamColor;
            _cache->driverStandings.push_back(ds);
        }
    }

    // Post-process: normalize all names to proper case (catches OpenF1 uppercase too)
    for (auto &ds : _cache->driverStandings) {
        char proper[24];
        properCase(proper, sizeof(proper), ds.driver.lastName);
        strlcpy(ds.driver.lastName, proper, sizeof(ds.driver.lastName));
        String full = String(ds.driver.firstName) + " " + String(proper);
        strlcpy(ds.driver.fullName, full.c_str(), sizeof(ds.driver.fullName));
    }

    // Sort by points descending (position 0 entries at bottom)
    std::stable_sort(_cache->driverStandings.begin(), _cache->driverStandings.end(),
              [](const DriverStanding &a, const DriverStanding &b) {
                  if ((a.position == 0) != (b.position == 0))
                      return a.position > 0; // drivers with position=0 at end
                  return a.points > b.points;
              });

    for (size_t i = 0; i < _cache->driverStandings.size(); i++)
        _cache->driverStandings[i].position = (int)(i + 1);

    Serial.printf("[SYNC] %zu total drivers (Jolpica + OpenF1 merged)\n", _cache->driverStandings.size());
    return true;
}

// Persistent constructor team color file (survives reboots, used when OpenF1 is unavailable)
static const char *TEAM_CLR_PATH = "/team_clr.bin";
static const uint32_t TEAM_CLR_MAGIC = 0x434C5254; // "TCLR"

static void saveConstructorColors(const std::vector<ConstructorStanding> &standings)
{
    File f = LittleFS.open(TEAM_CLR_PATH, FILE_WRITE);
    if (!f) return;
    f.write((uint8_t *)&TEAM_CLR_MAGIC, sizeof(TEAM_CLR_MAGIC));
    size_t count = standings.size();
    f.write((uint8_t *)&count, sizeof(count));
    for (auto &cs : standings) {
        f.write((uint8_t *)cs.team.id, sizeof(cs.team.id));
        f.write((uint8_t *)&cs.team.teamColor, sizeof(cs.team.teamColor));
    }
    f.close();
    Serial.printf("[SYNC] Saved %zu constructor colors to %s\n", count, TEAM_CLR_PATH);
}

static bool loadConstructorColors(std::vector<ConstructorStanding> &standings)
{
    File f = LittleFS.open(TEAM_CLR_PATH, FILE_READ);
    if (!f) return false;
    uint32_t magic;
    if (f.read((uint8_t *)&magic, sizeof(magic)) != sizeof(magic) || magic != TEAM_CLR_MAGIC) {
        f.close(); return false;
    }
    size_t count;
    if (f.read((uint8_t *)&count, sizeof(count)) != sizeof(count)) {
        f.close(); return false;
    }
    int applied = 0;
    for (size_t i = 0; i < count; i++) {
        char id[32] = {};
        uint16_t color;
        if (f.read((uint8_t *)id, sizeof(id)) != sizeof(id)) break;
        if (f.read((uint8_t *)&color, sizeof(color)) != sizeof(color)) break;
        for (auto &cs : standings) {
            if (strcmp(cs.team.id, id) == 0) {
                cs.team.teamColor = color;
                applied++;
                break;
            }
        }
    }
    f.close();
    Serial.printf("[SYNC] Restored %d constructor colors from %s\n", applied, TEAM_CLR_PATH);
    return applied > 0;
}

bool APIClient::syncConstructors()
{
    String season = String(_cache->currentSeason);
    Serial.printf("[SYNC] Constructors for %s...\n", season.c_str());

    String json;
    {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, String(JOLPICA_API_BASE) + "/" + season + "/constructorstandings.json");
        http.setTimeout(10000);
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
        cs.points = s["points"].as<float>();
        strlcpy(cs.team.name, s["Constructor"]["name"] | "", sizeof(cs.team.name));
        strlcpy(cs.team.id, s["Constructor"]["constructorId"] | "", sizeof(cs.team.id));

        // Match by constructor ID (case-insensitive) against driver team names
        cs.team.teamColor = UI::COL_TEAM_GREY;
        {
            char idPat[32];
            strlcpy(idPat, cs.team.id, sizeof(idPat));
            for (char *p = idPat; *p; p++) if (*p == '_') *p = ' ';

            for (const auto &ds : _cache->driverStandings) {
                bool match = false;
                const char *h = ds.driver.team.name;
                while (*h && !match) {
                    const char *a = h, *b = idPat;
                    while (*a && *b && tolower(*a) == tolower(*b)) { a++; b++; }
                    if (*b == '\0') match = true;
                    h++;
                }
                if (match) {
                    cs.team.teamColor = ds.driver.team.teamColor;
                    break;
                }
            }
        }
        _cache->constructorStandings.push_back(cs);
    }

    // Check if OpenF1 provided real colors; if so, save for next time
    bool hasRealColors = false;
    for (auto &cs : _cache->constructorStandings)
        if (cs.team.teamColor != 0x4208 && cs.team.teamColor != 0)
            { hasRealColors = true; break; }

    if (hasRealColors) {
        saveConstructorColors(_cache->constructorStandings);
    } else {
        // Restore previously saved colors (e.g., when OpenF1 is auth-restricted)
        loadConstructorColors(_cache->constructorStandings);
    }

    return true;
}

bool APIClient::fetchSessionResults(int round, const char *sessionType, std::vector<SessionResult> &results)
{
    String season = String(_cache->currentSeason);
    String endpoint;

    if (strcmp(sessionType, "Race") == 0)
        endpoint = "results";
    else if (strcmp(sessionType, "Qualifying") == 0)
        endpoint = "qualifying";
    else if (strcmp(sessionType, "Sprint") == 0)
        endpoint = "sprint";
    else
        return false;

    String url = String(JOLPICA_API_BASE) + "/" + season + "/" + String(round) + "/" + endpoint + ".json";
    Serial.printf("[FETCH] Session results: %s\n", url.c_str());

    String json;
    {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, url);
        http.setTimeout(10000);
        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
            Serial.printf("[FETCH] HTTP failed: %d\n", httpCode);
            return false;
        }
        json = http.getString();
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[FETCH] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray races = doc["MRData"]["RaceTable"]["Races"];
    if (races.size() == 0) return false;

    JsonVariant resultsArr;
    if (endpoint == "qualifying")
        resultsArr = races[0]["QualifyingResults"];
    else
        resultsArr = races[0]["Results"];

    if (!resultsArr.is<JsonArray>()) return false;

    results.clear();
    results.reserve(resultsArr.as<JsonArray>().size());

    for (JsonObject r : resultsArr.as<JsonArray>())
    {
        SessionResult sr;
        sr.position = r["position"].as<int>();
        sr.grid = r["grid"] | 0;
        sr.laps = r["laps"] | 0;
        sr.points = r["points"].as<float>();

        const char *code = r["Driver"]["code"] | "";
        strlcpy(sr.driverCode, code, sizeof(sr.driverCode));
        strlcpy(sr.constructorName, r["Constructor"]["name"] | "", sizeof(sr.constructorName));

        if (endpoint == "qualifying")
        {
            // Best lap time: Q3 > Q2 > Q1
            const char *q = r["Q3"] | r["Q2"] | r["Q1"] | "";
            strlcpy(sr.timeOrStatus, q, sizeof(sr.timeOrStatus));
            sr.isFastestLap = false;
        }
        else
        {
            const char *status = r["status"] | "";
            if (r["Time"]["time"].is<const char *>())
            {
                const char *t = r["Time"]["time"];
                // P1 gets total time like "1:28:15.486", others get gap like "+10.768"
                strlcpy(sr.timeOrStatus, t, sizeof(sr.timeOrStatus));
            }
            else
            {
                strlcpy(sr.timeOrStatus, status, sizeof(sr.timeOrStatus));
            }
            sr.isFastestLap = (r["FastestLap"]["rank"].as<int>() == 1);
        }

        results.push_back(sr);
    }

    Serial.printf("[FETCH] %zu results for %s (Rd %d)\n", results.size(), endpoint.c_str(), round);
    return true;
}

bool APIClient::syncCalendar()
{
    String season = String(_cache->currentSeason);
    Serial.printf("[SYNC] Calendar for %s...\n", season.c_str());

    String raceJson;
    {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, String(JOLPICA_API_BASE) + "/" + season + ".json");
        http.setTimeout(10000);
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
        strlcpy(rm.date, r["date"] | "", sizeof(rm.date));
        strlcpy(rm.officialName, r["raceName"] | "", sizeof(rm.officialName));
        strlcpy(rm.circuit.shortName, r["Circuit"]["circuitName"] | "", sizeof(rm.circuit.shortName));
        strlcpy(rm.circuit.location, r["Circuit"]["Location"]["locality"] | "", sizeof(rm.circuit.location));
        strlcpy(rm.circuit.countryName, r["Circuit"]["Location"]["country"] | "", sizeof(rm.circuit.countryName));

        // Parse embedded sessions from Jolpica
        rm.sessionCount = 0;
        memset(rm.sessions, 0, sizeof(rm.sessions));
        for (int si = 0; si < 6 && rm.sessionCount < 6; si++)
        {
            JsonVariant sess = r[sKeys[si]];
            if (!sess.isNull())
            {
                strlcpy(rm.sessions[rm.sessionCount].name, sNames[si], sizeof(rm.sessions[rm.sessionCount].name));
                snprintf(rm.sessions[rm.sessionCount].dateUtc, sizeof(rm.sessions[rm.sessionCount].dateUtc), "%sT%s",
                    sess["date"] | "",
                    sess["time"] | "");
                rm.sessionCount++;
            }
        }
        // Always add Race from top-level date/time
        if (rm.sessionCount < 6)
        {
            strlcpy(rm.sessions[rm.sessionCount].name, "Race", sizeof(rm.sessions[rm.sessionCount].name));
            snprintf(rm.sessions[rm.sessionCount].dateUtc, sizeof(rm.sessions[rm.sessionCount].dateUtc), "%sT%s",
                r["date"] | "",
                r["time"] | "");
            rm.sessionCount++;
        }

        _cache->calendar.push_back(rm);
    }

    // ── Fetch OpenF1 meeting keys (stream parsed) ──────────────────────────
    {
        WiFiClientSecure wcs;
        wcs.setInsecure();
        HTTPClient http;
        http.begin(wcs, String(OPENF1_API_BASE) + "/meetings?year=" + season);
        http.setTimeout(10000);
        int code = http.GET();
        if (code == HTTP_CODE_OK) {
            WiFiClient *stream = http.getStreamPtr();
            int resolved = 0;

            char buf[64];
            int bi = 0;
            enum { LOOK_KEY, READ_KEY, SKIP_VAL, READ_VAL } st = LOOK_KEY;
            char curKey[24] = "";

            int  cur_mk = 0;
            char cur_name[32] = "";
            char cur_date[12] = "";

            unsigned long timeout = millis() + 12000;
            while (stream->connected() && millis() < timeout) {
                while (stream->available()) {
                    char c = stream->read();

                    if (c == '{') {
                        cur_mk = 0; cur_name[0] = '\0'; cur_date[0] = '\0';
                        st = LOOK_KEY; bi = 0;
                        continue;
                    }
                    if (c == '}') {
                        if (cur_mk && cur_date[0] && strstr(cur_name, "Testing") == nullptr) {
                            int my, mm2, md;
                            if (sscanf(cur_date, "%d-%d-%d", &my, &mm2, &md) == 3) {
                                struct tm mt = {0};
                                mt.tm_year = my - 1900; mt.tm_mon = mm2 - 1; mt.tm_mday = md;
                                time_t meetT = mktime(&mt);
                                for (auto &rm : _cache->calendar) {
                                    int ry, rmo, rd;
                                    if (sscanf(rm.date, "%d-%d-%d", &ry, &rmo, &rd) < 3) continue;
                                    struct tm rt = {0};
                                    rt.tm_year = ry - 1900; rt.tm_mon = rmo - 1; rt.tm_mday = rd;
                                    time_t raceT = mktime(&rt);
                                    if (meetT >= raceT - 7 * 86400 && meetT <= raceT) {
                                        rm.meetingKey = cur_mk;
                                        resolved++;
                                        break;
                                    }
                                }
                            }
                        }
                        st = LOOK_KEY; bi = 0;
                        continue;
                    }

                    switch (st) {
                    case LOOK_KEY:
                        if (c == '"') { bi = 0; st = READ_KEY; }
                        break;
                    case READ_KEY:
                        if (c == '"') {
                            buf[bi] = '\0';
                            strlcpy(curKey, buf, sizeof(curKey));
                            if (strcmp(curKey, "meeting_key") == 0 ||
                                strcmp(curKey, "meeting_name") == 0 ||
                                strcmp(curKey, "date_start") == 0) {
                                st = READ_VAL; bi = 0;
                            } else {
                                st = SKIP_VAL;
                            }
                        } else if (bi < (int)sizeof(buf) - 1) buf[bi++] = c;
                        break;
                    case SKIP_VAL:
                        if (c == ',') st = LOOK_KEY;
                        else if (c == '}') st = LOOK_KEY;
                        break;
                    case READ_VAL:
                        if (c == ':' || c == ' ' || c == '\t') break;
                        if (c == '"') {
                            bi = 0;
                            unsigned long t2 = millis() + 2000;
                            while (millis() < t2) {
                                if (!stream->available()) { delay(1); continue; }
                                char c2 = stream->read();
                                if (c2 == '"') break;
                                if (bi < (int)sizeof(buf) - 1) buf[bi++] = c2;
                            }
                            buf[bi] = '\0';
                            if (strcmp(curKey, "meeting_name") == 0)
                                strlcpy(cur_name, buf, sizeof(cur_name));
                            else if (strcmp(curKey, "date_start") == 0) {
                                strncpy(cur_date, buf, 10); cur_date[10] = '\0';
                            }
                            st = LOOK_KEY;
                        } else {
                            bi = 0; buf[bi++] = c;
                            unsigned long t2 = millis() + 2000;
                            while (millis() < t2) {
                                if (!stream->available()) { delay(1); continue; }
                                char c2 = stream->peek();
                                if (c2 == ',' || c2 == '}' || c2 == ']' || c2 == ' ') break;
                                stream->read();
                                if (bi < (int)sizeof(buf) - 1) buf[bi++] = c2;
                            }
                            buf[bi] = '\0';
                            if (strcmp(curKey, "meeting_key") == 0)
                                cur_mk = atoi(buf);
                            st = LOOK_KEY;
                        }
                        break;
                    }
                }
                if (!stream->available()) delay(2);
            }
            http.end();
            Serial.printf("[SYNC] Meeting keys resolved: %d / %d races\n", resolved, _cache->calendar.size());
        } else {
            http.end();
            Serial.printf("[SYNC] OpenF1 meetings fetch failed: %d\n", code);
        }
    }

    int totalSessions = 0;
    for (auto &rm : _cache->calendar) totalSessions += rm.sessionCount;
    Serial.printf("[SYNC] Calendar complete. %d races, %d sessions.\n", _cache->calendar.size(), totalSessions);
    return true;
}

// ── RSS Helpers ────────────────────────────────────────────────────────

static void trimTrailing(char *str)
{
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == ']'))
        str[--len] = 0;
}

static void stripCDATA(char *str)
{
    const char *prefix = "<![CDATA[";
    char *p = strstr(str, prefix);
    if (!p) return;
    char *start = p + 9;
    char *end = strstr(start, "]]>");
    if (!end) return;
    memmove(p, start, end - start);
    p[end - start] = '\0';
}

static void extractRSSField(const char *xml, const char *tag, char *out, size_t outSize)
{
    char openTag[64], closeTag[64];
    snprintf(openTag, sizeof(openTag), "<%s>", tag);
    snprintf(closeTag, sizeof(closeTag), "</%s>", tag);

    const char *start = strstr(xml, openTag);
    if (!start) { out[0] = 0; return; }
    start += strlen(openTag);

    const char *end = strstr(start, closeTag);
    if (!end) { out[0] = 0; return; }

    size_t len = end - start;
    if (len >= outSize) len = outSize - 1;
    strncpy(out, start, len);
    out[len] = 0;

    stripCDATA(out);
}

static void extractRSSDescription(const char *xml, const char *tag, char *out, size_t outSize)
{
    char openTag[64], closeTag[64];
    snprintf(openTag, sizeof(openTag), "<%s>", tag);
    snprintf(closeTag, sizeof(closeTag), "</%s>", tag);

    const char *start = strstr(xml, openTag);
    if (!start) { out[0] = 0; return; }
    start += strlen(openTag);

    const char *end = strstr(start, closeTag);
    if (!end) { out[0] = 0; return; }

    size_t wi = 0;
    bool cdataOpen = false;
    bool inTag = false;
    bool inSpace = false;
    for (const char *r = start; r < end && wi < outSize - 1; r++)
    {
        char c = *r;

        if (c == '<')
        {
            if (strncmp(r, "<![CDATA[", 9) == 0)
            {
                cdataOpen = true;
                r += 8;
                continue;
            }
            inTag = true;
            continue;
        }
        if (c == '>')
        {
            inTag = false;
            continue;
        }
        if (inTag) continue;

        if (c == '&')
        {
            if      (strncmp(r, "&amp;",  5) == 0) { c = '&';  r += 4; }
            else if (strncmp(r, "&lt;",   4) == 0) { c = '<';  r += 3; }
            else if (strncmp(r, "&gt;",   4) == 0) { c = '>';  r += 3; }
            else if (strncmp(r, "&quot;", 6) == 0) { c = '"';  r += 5; }
            else if (strncmp(r, "&#39;",  5) == 0) { c = '\''; r += 4; }
            else if (strncmp(r, "&nbsp;", 6) == 0) { c = ' ';  r += 5; }
            else { continue; }
        }

        if (cdataOpen && c == ']' && strncmp(r, "]]>", 3) == 0)
        {
            r += 2;
            cdataOpen = false;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            if (!inSpace) { out[wi++] = ' '; inSpace = true; }
        }
        else
        {
            out[wi++] = c;
            inSpace = false;
        }
    }
    while (wi > 0 && (out[wi - 1] == ' ' || out[wi - 1] == ']')) wi--;
    out[wi] = 0;

    char *kr = strstr(out, "Keep reading");
    if (kr)
    {
        *kr = '\0';
        while (kr > out && kr[-1] == ' ') kr--;
        *kr = '\0';
    }
}

static void extractRSSDate(const char *xml, const char *tag, char *out, size_t outSize)
{
    char raw[64];
    extractRSSField(xml, tag, raw, sizeof(raw));
    if (raw[0] == 0) { out[0] = 0; return; }

    int day, year, hour, min;
    char month[8];
    if (sscanf(raw, "%*3s, %d %3s %d %d:%d", &day, month, &year, &hour, &min) >= 4)
        snprintf(out, outSize, "%d %s, %02d:%02d", day, month, hour, min);
    else
        strlcpy(out, raw, outSize);
}

bool APIClient::fetchNewsFeed()
{
    Serial.println("[RSS] Fetching motorsport.com news...");
    _cache->newsFeed.clear();

    bool success = false;
    for (int attempt = 0; attempt < 2; attempt++)
    {
        if (attempt > 0) delay(1000);

        WiFiClientSecure wcs;
        wcs.setInsecure();
        HTTPClient http;
        http.begin(wcs, RSS_NEWS_URL);
        http.setTimeout(10000);
        http.addHeader("User-Agent", "Mozilla/5.0 (compatible; F1Widget/1.0)");
        int code = http.GET();
        if (code == HTTP_CODE_OK)
        {
            WiFiClient *stream = http.getStreamPtr();
            if (!stream) { http.end(); continue; }

            int count = 0;
            // 2500 bytes is enough for a single <item> block (usually ~1.3KB)
            char itemBuf[2500];

            while (stream->connected() && count < 20) {
                // stream->find() returns true if the string is found
                // Note: find() consumes the stream up to and including the target
                if (!stream->find("<item>")) break;

                int idx = 0;
                unsigned long tStart = millis();
                bool foundEnd = false;

                // Read into buffer until we see </item> or fill buffer
                while (stream->connected() && (millis() - tStart) < 5000 && idx < (int)sizeof(itemBuf) - 1) {
                    if (stream->available()) {
                        char c = stream->read();
                        itemBuf[idx++] = c;
                        
                        // Check if the last 7 chars match "</item>"
                        if (idx >= 7 && strncmp(&itemBuf[idx-7], "</item>", 7) == 0) {
                            foundEnd = true;
                            break;
                        }
                    } else {
                        delay(1);
                    }
                }
                itemBuf[idx] = '\0';

                if (foundEnd) {
                    NewsArticle a;
                    memset(&a, 0, sizeof(NewsArticle));

                    extractRSSField(itemBuf, "title", a.title, sizeof(a.title));
                    extractRSSDescription(itemBuf, "description", a.description, sizeof(a.description));
                    extractRSSDate(itemBuf, "pubDate", a.pubDate, sizeof(a.pubDate));
                    extractRSSField(itemBuf, "link", a.url, sizeof(a.url));

                    if (strlen(a.title) > 0)
                    {
                        _cache->newsFeed.push_back(a);
                        count++;
                    }
                }
            }
            
            http.end();
            Serial.printf("[RSS] %d articles parsed\n", count);
            if (count > 0) success = true;
            break; // Break the retry loop if we succeeded or parsed the stream
        }
        else
        {
            http.end();
            Serial.printf("[RSS] Attempt %d: HTTP %d\n", attempt + 1, code);
        }
    }

    _cache->newsFeed.shrink_to_fit();
    return success;
}

// ═══════════════════════════════════════════════════════════════════════
// Circuit Outline + Lap Data Sync
// ═══════════════════════════════════════════════════════════════════════

// ── Cache path helpers ───────────────────────────────────────────────

static void _lapCachePath(int round, char *path, size_t sz)
{
    snprintf(path, sz, "/circuits/lap_R%02d.bin", round);
    for (char *p = path; *p; p++) { if (*p == '/') *p = '_'; }
}

static void _outlineCachePath(const char *shortName, char *path, size_t sz)
{
    snprintf(path, sz, "/circuits/%s.bin", shortName);
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '_';
        if (*p == ' ') *p = '_';
    }
}

static bool _hasCachedLap(int round)
{
    char path[32];
    _lapCachePath(round, path, sizeof(path));
    return LittleFS.exists(path);
}

static bool _hasCachedOutline(const char *shortName)
{
    char path[64];
    _outlineCachePath(shortName, path, sizeof(path));
    return LittleFS.exists(path);
}

// ── Lap cache binary I/O ─────────────────────────────────────────────

static void _saveLapCache(int round, const CachedLapData &data)
{
    char path[32];
    _lapCachePath(round, path, sizeof(path));
    File f = LittleFS.open(path, FILE_WRITE);
    if (!f) return;
    uint16_t magic = 0xC19A;
    f.write((uint8_t *)&magic, 2);
    f.write((uint8_t *)&data, sizeof(CachedLapData));
    f.close();
}

// ── Circuit outline binary I/O ───────────────────────────────────────

static void _saveCircuitCache(const char *shortName, const std::vector<CircuitPoint> &pts)
{
    char fname[64];
    _outlineCachePath(shortName, fname, sizeof(fname));
    File f = LittleFS.open(fname, FILE_WRITE);
    if (!f) return;
    uint16_t magic = 0xC199;
    uint16_t count = pts.size();
    f.write((uint8_t *)&magic, 2);
    f.write((uint8_t *)&count, 2);
    f.write((uint8_t *)pts.data(), count * sizeof(CircuitPoint));
    f.close();
    Serial.printf("[CIR] Cached %s: %d pts\n", fname, count);
}

// ── Session-level lap data probe (one cheap call) ────────────────────
// Returns: 1=has data, 0=empty, -1=404/error

static int _probeLapData(int sessionKey)
{
    WiFiClientSecure wcs;
    wcs.setInsecure();
    HTTPClient http;
    http.begin(wcs, String(OPENF1_API_BASE) + "/laps?session_key=" + String(sessionKey) + "&lap_number=1");
    http.setTimeout(8000);
    int code = http.GET();
    if (code != HTTP_CODE_OK) { http.end(); return -1; }
    String body = http.getString();
    http.end();
    body.trim();
    return (body == "[]" || body.length() < 5) ? 0 : 1;
}

// ── Stream-parse fastest lap for one driver ──────────────────────────
// Avoids loading 37KB into RAM by reading the stream and tracking the
// best lap_duration seen, capturing only the fields we need.

static bool _fetchFastestLapStream(int sessionKey, int driverNumber,
    float &lapTime, float &s1, float &s2, float &s3,
    int &speedTrap, int &i1Speed, int &i2Speed, int &lapNumber,
    char *lapDateStart, size_t dateBufSize)
{
    WiFiClientSecure wcs;
    wcs.setInsecure();
    HTTPClient http;
    String url = String(OPENF1_API_BASE) + "/laps?session_key=" + String(sessionKey)
               + "&driver_number=" + String(driverNumber);
    http.begin(wcs, url);
    http.setTimeout(15000);
    int code = http.GET();
    if (code != HTTP_CODE_OK) { http.end(); return false; }

    WiFiClient *stream = http.getStreamPtr();
    if (!stream) { http.end(); return false; }

    // Fields we track per lap object
    float cur_dur = 0, cur_s1 = 0, cur_s2 = 0, cur_s3 = 0;
    int   cur_st = 0, cur_i1 = 0, cur_i2 = 0, cur_ln = 0;
    bool  cur_pitout = false;
    char  cur_date[48] = "";

    // Best lap so far
    float best = 1e9f;
    bool  found = false;
    lapTime = 1e9f;

    char  buf[64];
    int   bi = 0;
    char  curKey[32] = "";
    enum  { LOOK_KEY, READ_KEY, SKIP_VAL, READ_STR_VAL, READ_NUM_VAL } st = LOOK_KEY;
    int   depth = 0; // bracket/brace nesting

    unsigned long timeout = millis() + 18000;
    while (stream->connected() && millis() < timeout) {
        while (stream->available() && millis() < timeout) {
            char c = stream->read();

            if (c == '[') { depth++; continue; }
            if (c == ']') { depth--; continue; }

            if (c == '{') {
                // New lap object
                cur_dur = cur_s1 = cur_s2 = cur_s3 = 0;
                cur_st = cur_i1 = cur_i2 = cur_ln = 0;
                cur_pitout = false; cur_date[0] = '\0';
                st = LOOK_KEY; bi = 0;
                continue;
            }
            if (c == '}') {
                // Lap object ended — check if it's the best non-pit lap
                if (cur_dur > 0 && !cur_pitout && cur_dur < best) {
                    best = cur_dur;
                    lapTime  = cur_dur; s1 = cur_s1; s2 = cur_s2; s3 = cur_s3;
                    speedTrap = cur_st; i1Speed = cur_i1; i2Speed = cur_i2;
                    lapNumber = cur_ln;
                    if (lapDateStart && dateBufSize > 0)
                        strlcpy(lapDateStart, cur_date, dateBufSize);
                    found = true;
                }
                st = LOOK_KEY; bi = 0;
                continue;
            }

            switch (st) {
            case LOOK_KEY:
                if (c == '"') { bi = 0; st = READ_KEY; }
                break;
            case READ_KEY:
                if (c == '"') {
                    buf[bi] = '\0';
                    strlcpy(curKey, buf, sizeof(curKey));
                    // Decide how to handle value
                    bool isStr = (strcmp(curKey,"date_start")==0);
                    bool isNum = (strcmp(curKey,"lap_duration")==0 ||
                                  strcmp(curKey,"duration_sector_1")==0 ||
                                  strcmp(curKey,"duration_sector_2")==0 ||
                                  strcmp(curKey,"duration_sector_3")==0 ||
                                  strcmp(curKey,"st_speed")==0 ||
                                  strcmp(curKey,"i1_speed")==0 ||
                                  strcmp(curKey,"i2_speed")==0 ||
                                  strcmp(curKey,"lap_number")==0 ||
                                  strcmp(curKey,"is_pit_out_lap")==0);
                    st = isStr ? READ_STR_VAL : (isNum ? READ_NUM_VAL : SKIP_VAL);
                    bi = 0;
                } else if (bi < (int)sizeof(buf)-1) buf[bi++] = c;
                break;
            case SKIP_VAL:
                // Skip until next comma or closing brace at same depth
                if (c == ',' || c == '}') {
                    if (c == '}') {
                        // re-process the closing brace
                        if (cur_dur > 0 && !cur_pitout && cur_dur < best) {
                            best = cur_dur;
                            lapTime  = cur_dur; s1 = cur_s1; s2 = cur_s2; s3 = cur_s3;
                            speedTrap = cur_st; i1Speed = cur_i1; i2Speed = cur_i2;
                            lapNumber = cur_ln;
                            if (lapDateStart && dateBufSize > 0)
                                strlcpy(lapDateStart, cur_date, dateBufSize);
                            found = true;
                        }
                        st = LOOK_KEY;
                    } else {
                        st = LOOK_KEY;
                    }
                } else if (c == '[') {
                    // Skip arrays (segments_sector_*)
                    int arr = 1;
                    unsigned long at = millis() + 3000;
                    while (arr > 0 && stream->connected() && millis() < at) {
                        if (!stream->available()) { delay(1); continue; }
                        char ac = stream->read();
                        if (ac == '[') arr++;
                        else if (ac == ']') arr--;
                    }
                    st = LOOK_KEY;
                }
                break;
            case READ_STR_VAL: {
                // Find opening quote, read until closing quote
                if (c == ':' || c == ' ') break;
                if (c == '"') {
                    bi = 0;
                    unsigned long t2 = millis() + 2000;
                    while (millis() < t2) {
                        if (!stream->available()) { delay(1); continue; }
                        char c2 = stream->read();
                        if (c2 == '"') break;
                        if (bi < (int)sizeof(buf)-1) buf[bi++] = c2;
                    }
                    buf[bi] = '\0';
                    if (strcmp(curKey,"date_start")==0) {
                        // Trim subseconds/timezone, keep YYYY-MM-DDTHH:MM:SS
                        char *dot = strchr(buf, '.');
                        if (dot) *dot = '\0';
                        strlcpy(cur_date, buf, sizeof(cur_date));
                    }
                    st = LOOK_KEY;
                }
                break;
            }
            case READ_NUM_VAL: {
                if (c == ':' || c == ' ') break;
                bi = 0; buf[bi++] = c;
                unsigned long t2 = millis() + 2000;
                while (millis() < t2) {
                    if (!stream->available()) { delay(1); continue; }
                    char c2 = stream->peek();
                    if (c2==',' || c2=='}' || c2==']' || c2==' ' || c2=='\n') break;
                    stream->read();
                    if (bi < (int)sizeof(buf)-1) buf[bi++] = c2;
                }
                buf[bi] = '\0';
                float fv = atof(buf); int iv = atoi(buf);
                if      (strcmp(curKey,"lap_duration")==0)       cur_dur = fv;
                else if (strcmp(curKey,"duration_sector_1")==0)  cur_s1  = fv;
                else if (strcmp(curKey,"duration_sector_2")==0)  cur_s2  = fv;
                else if (strcmp(curKey,"duration_sector_3")==0)  cur_s3  = fv;
                else if (strcmp(curKey,"st_speed")==0)           cur_st  = iv;
                else if (strcmp(curKey,"i1_speed")==0)           cur_i1  = iv;
                else if (strcmp(curKey,"i2_speed")==0)           cur_i2  = iv;
                else if (strcmp(curKey,"lap_number")==0)         cur_ln  = iv;
                else if (strcmp(curKey,"is_pit_out_lap")==0)     cur_pitout = (iv != 0 || buf[0]=='t');
                st = LOOK_KEY;
                break;
            }
            }
        }
        if (!stream->available()) delay(2);
    }
    http.end();
    return found;
}

// ── Session result (finishing position, gap, laps) ───────────────────

static bool _fetchSessionResult(int sessionKey, int driverNumber,
    int &position, float &gapToLeader, int &numLaps)
{
    WiFiClientSecure wcs;
    wcs.setInsecure();
    HTTPClient http;
    String url = String(OPENF1_API_BASE) + "/session_result?session_key=" + String(sessionKey)
               + "&driver_number=" + String(driverNumber);
    http.begin(wcs, url);
    http.setTimeout(8000);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[CIR] session_result query failed: %d\n", code);
        http.end(); return false;
    }
    String body = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, body) || !doc.is<JsonArray>()) return false;
    JsonArray arr = doc.as<JsonArray>();
    if (arr.size() == 0) return false;

    JsonVariant r = arr[0];
    position = r["position"].as<int>();
    const char *gap = r["gap_to_leader"].as<const char *>();
    if (gap) {
        if (strstr(gap, "LAP") || strstr(gap, "lap")) gapToLeader = -1.0f;
        else gapToLeader = atof(gap + (gap[0] == '+' ? 1 : 0));
    }
    numLaps = r["number_of_laps"].as<int>();
    return true;
}

// ── Circuit outline stream fetch ──────────────────────────────────────

bool APIClient::fetchCircuitOutline(int sessionKey, int driverNumber,
    std::vector<CircuitPoint> &points, int maxPoints, const char *dateAfter)
{
    points.clear();
    WiFiClientSecure wcs;
    wcs.setInsecure();
    HTTPClient http;

    String url = String(OPENF1_API_BASE) + "/location?session_key=" + String(sessionKey)
               + "&driver_number=" + String(driverNumber);
    if (dateAfter) { url += "&date>="; url += dateAfter; }

    Serial.printf("[CIR] Fetching location: session_key=%d driver_number=%d%s\n",
        sessionKey, driverNumber, dateAfter ? " (date filtered)" : "");
    http.begin(wcs, url);
    http.setTimeout(15000);
    http.addHeader("Accept", "application/json");
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[CIR] location query failed: %d url=%s\n", code, url.c_str());
        http.end(); return false;
    }

    WiFiClient *stream = http.getStreamPtr();
    if (!stream) { http.end(); return false; }

    struct RawPt { float x, y; };
    std::vector<RawPt> raw;
    raw.reserve(500);

    char buf[32]; int bi = 0;
    enum { LOOK_KEY, READ_KEY, SKIP_VAL, READ_VAL_X, READ_VAL_Y } state = LOOK_KEY;
    float tmpX = 0, tmpY = 0;
    bool hasX = false, hasY = false;
    int totalParsed = 0;

    unsigned long timeout = millis() + 20000;
    while (stream->connected() && raw.size() < 600 && totalParsed < 50000 && millis() < timeout) {
        while (stream->available() && raw.size() < 600 && totalParsed < 50000) {
            int c = stream->read();
            if (c < 0) break;
            switch (state) {
            case LOOK_KEY:
                if (c == '"') { bi = 0; state = READ_KEY; }
                break;
            case READ_KEY:
                if (c == '"') {
                    buf[bi] = '\0';
                    if      (strcmp(buf,"x")==0) { state = READ_VAL_X; bi = 0; }
                    else if (strcmp(buf,"y")==0) { state = READ_VAL_Y; bi = 0; }
                    else                          state = SKIP_VAL;
                } else if (bi < (int)sizeof(buf)-1) buf[bi++] = c;
                break;
            case SKIP_VAL:
                if (c == '}') { state = LOOK_KEY; totalParsed++; }
                else if (c == ',') state = LOOK_KEY;
                break;
            case READ_VAL_X:
                if (c == ':') break;
                if (c == ',' || c == '}' || c == ']') {
                    buf[bi] = '\0'; tmpX = atof(buf); hasX = true; bi = 0;
                    state = LOOK_KEY;
                    if (hasY) {
                        if (tmpX != 0 || tmpY != 0) raw.push_back({tmpX, tmpY});
                        hasX = hasY = false;
                    }
                } else if (bi < (int)sizeof(buf)-1) buf[bi++] = c;
                break;
            case READ_VAL_Y:
                if (c == ':') break;
                if (c == ',' || c == '}' || c == ']') {
                    buf[bi] = '\0'; tmpY = atof(buf); hasY = true; bi = 0;
                    state = LOOK_KEY;
                    if (hasX) {
                        if (tmpX != 0 || tmpY != 0) raw.push_back({tmpX, tmpY});
                        hasX = hasY = false;
                    }
                } else if (bi < (int)sizeof(buf)-1) buf[bi++] = c;
                break;
            }
        }
        if (!stream->available()) delay(2);
    }
    http.end();

    int rawCount = raw.size();
    if (rawCount < 2) {
        Serial.printf("[CIR] Not enough location points: %d\n", rawCount);
        return false;
    }

    int step = (rawCount > maxPoints) ? (rawCount / maxPoints) : 1;
    points.push_back({(int16_t)raw[0].x, (int16_t)raw[0].y});
    for (int i = step; i < rawCount - 1; i += step)
        points.push_back({(int16_t)raw[i].x, (int16_t)raw[i].y});
    if (rawCount > 1) {
        CircuitPoint lp = {(int16_t)raw.back().x, (int16_t)raw.back().y};
        if (lp.x != points.back().x || lp.y != points.back().y)
            points.push_back(lp);
    }

    Serial.printf("[CIR] Outline first=(%d,%d) last=(%d,%d)\n",
        points[0].x, points[0].y,
        points[points.size()-1].x, points[points.size()-1].y);
    Serial.printf("[CIR] Circuit outline: %d points (from %d raw)\n", points.size(), rawCount);
    return !points.empty();
}

// ── Ordered driver candidate list (fav first, then standings order) ──

static int _buildDriverList(const DataCache *cache, int *out, int maxOut)
{
    int n = 0;
    // Championship order — P1 first (most likely to have complete data)
    for (auto &ds : cache->driverStandings) {
        if (n >= maxOut) break;
        out[n++] = ds.driver.number;
    }
    if (n == 0) { out[0] = 1; n = 1; }
    return n;
}

// ── Past-race guard ───────────────────────────────────────────────────

static bool _isRacePassed(const char *dateStr, time_t now)
{
    struct tm t = {0};
    if (sscanf(dateStr, "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday) < 3) return false;
    t.tm_year -= 1900; t.tm_mon -= 1;
    // 2-day buffer — OpenF1 publishes historical data a day or two after the race
    return (mktime(&t) + 2 * 86400) < now;
}

// ── Cache one round ───────────────────────────────────────────────────

bool APIClient::_cacheOneRound(int round, const RaceMeeting *rm)
{
    Serial.printf("[CIR] Caching round %d (%s)...\n", round, rm->circuit.shortName);

    if (!rm->meetingKey) {
        Serial.printf("[CIR] R%02d: no meeting_key (run Sync Data first)\n", round);
        return false;
    }

    // Resolve Race session_key from /v1/sessions?meeting_key=X (~2KB)
    int sessionKey = 0;
    {
        WiFiClientSecure wcs; wcs.setInsecure();
        HTTPClient http;
        http.begin(wcs, String(OPENF1_API_BASE) + "/sessions?meeting_key=" + String(rm->meetingKey));
        http.setTimeout(8000);
        if (http.GET() == HTTP_CODE_OK) {
            String body = http.getString(); http.end();
            JsonDocument sdoc;
            if (!deserializeJson(sdoc, body) && sdoc.is<JsonArray>()) {
                int qualKey = 0;
                for (JsonVariant s : sdoc.as<JsonArray>()) {
                    const char *sname = s["session_name"].as<const char *>();
                    int sk = s["session_key"].as<int>();
                    if (!sname || !sk) continue;
                    if (strcasecmp(sname, "Race") == 0) { sessionKey = sk; break; }
                    if (strcasecmp(sname, "Qualifying") == 0) qualKey = sk;
                }
                if (!sessionKey) sessionKey = qualKey;
            }
        } else { http.end(); }
    }
    if (!sessionKey) {
        Serial.printf("[CIR] R%02d: could not resolve session_key from meeting %d\n", round, rm->meetingKey);
        return false;
    }
    Serial.printf("[CIR] R%02d: meeting_key=%d session_key=%d\n", round, rm->meetingKey, sessionKey);

    // Probe: does this session have any lap data at all?
    int probe = _probeLapData(sessionKey);
    if (probe <= 0) {
        Serial.printf("[CIR] R%02d: no lap data yet (probe=%d), skipping\n", round, probe);
        // Still try outline — GPS data can exist before lap times are published
    }

    // Build ordered driver list (fav first, then championship order)
    int drivers[22];
    int numDrivers = _buildDriverList(_cache, drivers, 22);

    // ── Lap data ─────────────────────────────────────────────────────
    CachedLapData ld;
    memset(&ld, 0, sizeof(ld));

    if (probe > 0) {
        for (int i = 0; i < numDrivers && !ld.valid; i++) {
            int dn = drivers[i];
            float lt=0,s1=0,s2=0,s3=0; int st=0,i1=0,i2=0,ln=0;
            char dateBuf[48] = "";
            if (_fetchFastestLapStream(sessionKey, dn, lt, s1, s2, s3, st, i1, i2, ln, dateBuf, sizeof(dateBuf))) {
                ld.valid = true;
                ld.lapTime=lt; ld.s1=s1; ld.s2=s2; ld.s3=s3;
                ld.speedTrap=st; ld.i1Speed=i1; ld.i2Speed=i2; ld.lapNumber=ln;
                strlcpy(ld.lapDateStart, dateBuf, sizeof(ld.lapDateStart));
                int pos=0; float gap=0; int laps=0;
                if (_fetchSessionResult(sessionKey, dn, pos, gap, laps)) {
                    ld.position=(uint16_t)pos; ld.gapToLeader=gap; ld.numLaps=(uint16_t)laps;
                }
                Serial.printf("[CIR] R%02d lap data: driver=%d lap=%d time=%.3f\n", round, dn, ln, lt);
            }
        }
        if (!ld.valid)
            Serial.printf("[CIR] R%02d: no lap data from any driver\n", round);
    }

    // Only persist if valid — invalid rounds retry on next sync
    if (ld.valid) _saveLapCache(round, ld);

    // ── Circuit outline ───────────────────────────────────────────────
    if (_hasCachedOutline(rm->circuit.shortName)) {
        Serial.printf("[CIR] R%02d outline already cached\n", round);
        return ld.valid;
    }

    std::vector<CircuitPoint> rawPoints;
    bool outlineOk = false;
    const char *dateFilter = ld.lapDateStart[0] ? ld.lapDateStart : nullptr;

    if (dateFilter) {
        // Try fav driver with date filter first
        outlineOk = fetchCircuitOutline(sessionKey, drivers[0], rawPoints, 350, dateFilter);
        // If date filter fails immediately drop it — the filter is session-wide, not per-driver
        if (!outlineOk) {
            Serial.println("[CIR] Date filter failed, retrying without filter...");
            dateFilter = nullptr;
        }
    }

    if (!outlineOk) {
        // Try top-4 drivers without date filter
        int lim = (numDrivers < 4) ? numDrivers : 4;
        for (int i = 0; i < lim && !outlineOk; i++)
            outlineOk = fetchCircuitOutline(sessionKey, drivers[i], rawPoints, 350, nullptr);
    }

    if (outlineOk) _saveCircuitCache(rm->circuit.shortName, rawPoints);
    else Serial.printf("[CIR] R%02d: could not fetch circuit outline\n", round);

    return ld.valid || outlineOk;
}

// ── Background Circuit Sync ───────────────────────────────────────────

bool APIClient::syncCircuits()
{
    if (!_cache || _cache->calendar.empty()) return false;

    time_t now = time(nullptr);
    int synced = 0, skipped = 0, notReady = 0;

    for (auto &rm : _cache->calendar) {
        if (!_isRacePassed(rm.date, now)) continue;

        bool haveLap     = _hasCachedLap(rm.round);
        bool haveOutline = _hasCachedOutline(rm.circuit.shortName);

        if (haveLap && haveOutline) { skipped++; continue; }

        Serial.printf("[CIR] Round %d: lap=%s outline=%s — fetching\n",
            rm.round, haveLap ? "OK" : "missing", haveOutline ? "OK" : "missing");

        if (_cacheOneRound(rm.round, &rm)) synced++;
        else notReady++;
    }

    Serial.printf("[CIR] Circuit sync done: %d cached, %d skipped (complete), %d not ready\n",
        synced, skipped, notReady);
    return true;
}

