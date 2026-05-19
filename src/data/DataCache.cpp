#include "DataCache.h"
#include <LittleFS.h>

// Global cache instance
DataCache *cache = nullptr;

DataCache::DataCache()
{
    // Minimal pre-allocation
    driverStandings.reserve(20);
    constructorStandings.reserve(10);
    calendar.reserve(24);
}

void DataCache::begin()
{
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

const uint32_t CACHE_MAGIC = 0xF1CA0002; // Versioned magic

void DataCache::save()
{
    File file = LittleFS.open(CACHE_PATH, FILE_WRITE);
    if (!file)
        return;

    // 1. Save Magic and Metadata
    file.write((uint8_t *)&CACHE_MAGIC, sizeof(uint32_t));
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
    Serial.println("[CACHE] Saved successfully");
}

void DataCache::load()
{
    Serial.println("[CACHE] Loading...");
    if (!LittleFS.exists(CACHE_PATH)) {
        Serial.println("[CACHE] No cache file found");
        return;
    }
    File file = LittleFS.open(CACHE_PATH, FILE_READ);
    if (!file) {
        Serial.println("[CACHE] Failed to open file");
        return;
    }

    uint32_t magic = 0;
    file.read((uint8_t *)&magic, sizeof(uint32_t));
    if (magic != CACHE_MAGIC) {
        Serial.printf("[CACHE] Invalid magic: 0x%08X (Expected 0x%08X). Clearing.\n", magic, CACHE_MAGIC);
        file.close();
        clear();
        return;
    }

    file.read((uint8_t *)&currentSeason, sizeof(int));
    if (currentSeason < 2024 || currentSeason > 2030) {
        Serial.printf("[CACHE] Invalid season: %d. Clearing.\n", currentSeason);
        file.close();
        clear();
        return;
    }
    Serial.printf("[CACHE] Season: %d\n", currentSeason);

    // Load Drivers
    size_t dCount = 0;
    file.read((uint8_t *)&dCount, sizeof(size_t));
    if (dCount > 100) {
        Serial.println("[CACHE] Corrupt dCount");
        file.close(); clear(); return;
    }
    driverStandings.resize(dCount);
    if (dCount > 0)
        file.read((uint8_t *)driverStandings.data(), sizeof(DriverStanding) * dCount);

    // Load Constructors
    size_t cCount = 0;
    file.read((uint8_t *)&cCount, sizeof(size_t));
    if (cCount > 50) {
        Serial.println("[CACHE] Corrupt cCount");
        file.close(); clear(); return;
    }
    constructorStandings.resize(cCount);
    if (cCount > 0)
        file.read((uint8_t *)constructorStandings.data(), sizeof(ConstructorStanding) * cCount);

    // Load Calendar
    size_t mCount = 0;
    file.read((uint8_t *)&mCount, sizeof(size_t));
    if (mCount > 50) {
        Serial.println("[CACHE] Corrupt mCount");
        file.close(); clear(); return;
    }
    calendar.resize(mCount);
    if (mCount > 0)
        file.read((uint8_t *)calendar.data(), sizeof(RaceMeeting) * mCount);

    file.close();
    Serial.println("[CACHE] Load complete");
}

void DataCache::clear()
{
    driverStandings.clear();
    constructorStandings.clear();
    calendar.clear();
    if (LittleFS.exists(CACHE_PATH))
        LittleFS.remove(CACHE_PATH);
}