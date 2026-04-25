#ifndef DATA_CACHE_H
#define DATA_CACHE_H

#include <Arduino.h>

struct DriverStanding
{
    int position;
    int points;
    char firstName[32];
    char lastName[32];
    char code[4];
    char teamName[64];
};

struct ConstructorStanding
{
    int position;
    int points;
    char name[64];
};

class DataCache
{
public:
    DataCache();

    // --- Drivers Section ---
    static constexpr int MAX_DRIVERS = 22;
    DriverStanding drivers[MAX_DRIVERS];
    int driverCount = 0; // Only declare this ONCE
    unsigned long driversLastUpdated = 0;

    // --- Constructors Section ---
    static constexpr int MAX_CONSTRUCTORS = 11;
    ConstructorStanding constructors[MAX_CONSTRUCTORS];
    int constructorCount = 0; // Only declare this ONCE
    unsigned long constructorsLastUpdated = 0;

    // Persistence Methods
    bool saveDrivers();
    bool loadDrivers();
    bool saveConstructors();
    bool loadConstructors();
};

extern DataCache *cache;

#endif