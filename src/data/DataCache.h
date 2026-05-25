#pragma once

#include <Arduino.h>
#include <vector>

class DataCache; // Forward declaration

// Global cache instance
extern DataCache *cache;

// ── DATA MODELS ──────────────────────────────────────────────────────

struct Team
{
    char id[32];
    char name[64];
    uint16_t teamColor;
};

struct Driver
{
    int number;
    char firstName[16];
    char lastName[24];
    char fullName[48];
    char broadcastName[24];
    char acronym[4];
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
    char shortName[48];
    char location[48];
    char countryName[24];
};

struct Session
{
    char name[32];    // "Practice 1", "Qualifying", "Race"
    char dateUtc[32]; // "2026-03-15T06:00:00Z"
};

struct SessionResult
{
    int position;
    int grid;
    int laps;
    int points;
    char driverCode[4];
    char constructorName[64];
    char timeOrStatus[28];
    bool isFastestLap;
};

struct NewsArticle
{
    char title[128];
    char description[384];
    char pubDate[24];
    char url[192];
};

struct RaceMeeting
{
    int round;
    int meetingKey;
    char officialName[96];
    char date[12];
    Circuit circuit;
    Session sessions[6];
    int sessionCount;
};

// ── DYNAMIC CACHE CONTROLLER ──────────────────────────────────────────

class DataCache
{
public:
    // Dynamic Containers
    std::vector<DriverStanding> driverStandings;
    std::vector<ConstructorStanding> constructorStandings;
    std::vector<RaceMeeting> calendar;
    std::vector<NewsArticle> newsFeed;

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