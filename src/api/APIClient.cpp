#include "APIClient.h"
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
        http.begin(client, "https://api.jolpi.ca/ergast/f1/" + season + "/driverstandings.json");
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
        http.begin(client, "https://api.openf1.org/v1/drivers?session_key=latest");
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
                        ds.driver.team.teamColor = 0x7BEF;
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
        http.begin(client, "https://api.jolpi.ca/ergast/f1/" + season + "/constructorstandings.json");
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
        cs.team.teamColor = 0x4208;
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

    String url = "https://api.jolpi.ca/ergast/f1/" + season + "/" + String(round) + "/" + endpoint + ".json";
    Serial.printf("[FETCH] Session results: %s\n", url.c_str());

    String json;
    {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.begin(client, url);
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
        http.begin(client, "https://api.jolpi.ca/ergast/f1/" + season + ".json");
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
        rm.meetingKey = 0;

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

    String body;
    {
        WiFiClientSecure wcs;
        wcs.setInsecure();
        HTTPClient http;
        http.begin(wcs, "https://www.motorsport.com/rss/f1/news/");
        http.addHeader("User-Agent", "Mozilla/5.0 (compatible; F1Widget/1.0)");
        int code = http.GET();
        if (code != HTTP_CODE_OK)
        {
            Serial.printf("[RSS] HTTP failed: %d\n", code);
            return false;
        }
        body = http.getString();
        Serial.printf("[RSS] Downloaded %d bytes\n", body.length());
    }

    if (body.length() < 100) {
        Serial.printf("[RSS] Body too short: %d bytes\n", body.length());
        return false;
    }

    const char *p = body.c_str();
    int count = 0;
    while (*p && count < 20)
    {
        p = strstr(p, "<item>");
        if (!p) break;
        p += 6;

        const char *end = strstr(p, "</item>");
        if (!end) break;

        NewsArticle a;
        memset(&a, 0, sizeof(NewsArticle));

        extractRSSField(p, "title", a.title, sizeof(a.title));
        extractRSSDescription(p, "description", a.description, sizeof(a.description));
        extractRSSDate(p, "pubDate", a.pubDate, sizeof(a.pubDate));
        extractRSSField(p, "link", a.url, sizeof(a.url));

        if (strlen(a.title) > 0)
        {
            _cache->newsFeed.push_back(a);
            count++;
        }

        p = end + 7;
    }

    Serial.printf("[RSS] %d articles parsed\n", count);
    _cache->newsFeed.shrink_to_fit();
    return count > 0;
}


