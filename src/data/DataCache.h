#pragma once

#include <Arduino.h>
#include <vector>

// ── DATA MODELS ──────────────────────────────────────────────────────

struct Team
{
    char id[32];
    char name[64];
    uint16_t teamColor; // Pre-calculated RGB565 from OpenF1 "team_colour"
};

struct Driver
{
    int number;
    char firstName[32];
    char lastName[32];
    char fullName[64];
    char broadcastName[32];
    char acronym[4];
    char headshotUrl[128];
    Team team;
};

struct DriverStanding
{
    int position;
    int points;
    int wins;
    Driver driver;
};

struct ConstructorStanding
{
    int position;
    int points;
    Team team;
};

struct Circuit
{
    int circuitKey;
    char shortName[64];
    char location[64];
    char countryName[32];
    char countryCode[4];
    char trackImageUrl[128];
};

struct RaceMeeting
{
    int round;      // <--- Add this for Jolpica round number
    int meetingKey; // From OpenF1
    char officialName[128];
    char dateStart[32];
    char dateEnd[32];
    char date[12]; // <--- Add this for Jolpica "YYYY-MM-DD"
    Circuit circuit;
};

// ── DYNAMIC CACHE CONTROLLER ──────────────────────────────────────────

class DataCache
{
public:
    // Dynamic Containers
    std::vector<DriverStanding> driverStandings;
    std::vector<ConstructorStanding> constructorStandings;
    std::vector<RaceMeeting> calendar;

    int currentSeason = 2026;
    uint32_t lastUpdated = 0;

    DataCache();

    void begin();
    void save();
    void load();
    void clear();

    // High-performance color utility
    static uint16_t hexTo565(const char *hexStr);

private:
    const char *CACHE_PATH = "/f1_master_cache.bin";
};