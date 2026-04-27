#include "DataCache.h"
#include <LittleFS.h>

DataCache::DataCache()
{
    // Pre-allocate typical F1 sizes to prevent fragmentation
    driverStandings.reserve(24);
    constructorStandings.reserve(12);
    calendar.reserve(25);
}

void DataCache::begin()
{
    if (!LittleFS.begin(true))
        return;
    load();
}

uint16_t DataCache::hexTo565(const char *hexStr)
{
    if (!hexStr || hexStr[0] == '\0')
        return 0x4208;
    uint32_t rgb = strtoul(hexStr, NULL, 16);
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void DataCache::save()
{
    File file = LittleFS.open(CACHE_PATH, FILE_WRITE);
    if (!file)
        return;

    // 1. Save Header/Metadata
    file.write((uint8_t *)&currentSeason, sizeof(int));

    // 2. Save Driver Standings
    size_t dCount = driverStandings.size();
    file.write((uint8_t *)&dCount, sizeof(size_t));
    if (dCount > 0)
    {
        file.write((uint8_t *)driverStandings.data(), sizeof(DriverStanding) * dCount);
    }

    // 3. Save Constructor Standings
    size_t cCount = constructorStandings.size();
    file.write((uint8_t *)&cCount, sizeof(size_t));
    if (cCount > 0)
    {
        file.write((uint8_t *)constructorStandings.data(), sizeof(ConstructorStanding) * cCount);
    }

    // 4. Save Calendar
    size_t mCount = calendar.size();
    file.write((uint8_t *)&mCount, sizeof(size_t));
    if (mCount > 0)
    {
        file.write((uint8_t *)calendar.data(), sizeof(RaceMeeting) * mCount);
    }

    file.close();
}

void DataCache::load()
{
    if (!LittleFS.exists(CACHE_PATH))
        return;
    File file = LittleFS.open(CACHE_PATH, FILE_READ);
    if (!file)
        return;

    file.read((uint8_t *)&currentSeason, sizeof(int));

    // Load Drivers
    size_t dCount;
    file.read((uint8_t *)&dCount, sizeof(size_t));
    driverStandings.resize(dCount);
    if (dCount > 0)
        file.read((uint8_t *)driverStandings.data(), sizeof(DriverStanding) * dCount);

    // Load Constructors
    size_t cCount;
    file.read((uint8_t *)&cCount, sizeof(size_t));
    constructorStandings.resize(cCount);
    if (cCount > 0)
        file.read((uint8_t *)constructorStandings.data(), sizeof(ConstructorStanding) * cCount);

    // Load Calendar
    size_t mCount;
    file.read((uint8_t *)&mCount, sizeof(size_t));
    calendar.resize(mCount);
    if (mCount > 0)
        file.read((uint8_t *)calendar.data(), sizeof(RaceMeeting) * mCount);

    file.close();
}

void DataCache::clear()
{
    driverStandings.clear();
    constructorStandings.clear();
    calendar.clear();
    if (LittleFS.exists(CACHE_PATH))
        LittleFS.remove(CACHE_PATH);
}