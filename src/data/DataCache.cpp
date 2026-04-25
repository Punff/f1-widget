#include "DataCache.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// 1. Define the global pointer (Fixes 'undefined reference' in main.cpp)
DataCache *cache = nullptr;

// 2. The Constructor
// Prefix 'DataCache::' is CRITICAL for the linker
DataCache::DataCache()
{
    driverCount = 0;
    constructorCount = 0;
    driversLastUpdated = 0;
    constructorsLastUpdated = 0;
}

// ─── DRIVER METHODS ───────────────────────────────────────────────────────

bool DataCache::loadDrivers()
{
    if (!LittleFS.exists("/drivers.json"))
        return false;
    File file = LittleFS.open("/drivers.json", "r");
    if (!file)
        return false;

    JsonDocument doc; // ArduinoJson 7 syntax
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        file.close();
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    driverCount = 0;
    for (JsonObject obj : arr)
    {
        if (driverCount >= MAX_DRIVERS)
            break;

        drivers[driverCount].position = obj["pos"] | 0;
        drivers[driverCount].points = obj["pts"] | 0;
        strlcpy(drivers[driverCount].lastName, obj["name"] | "", 32);
        driverCount++;
    }

    file.close();
    return true;
}

bool DataCache::saveDrivers()
{
    File file = LittleFS.open("/drivers.json", "w");
    if (!file)
        return false;

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < driverCount; i++)
    {
        JsonObject obj = arr.add<JsonObject>(); // V7 syntax
        obj["pos"] = drivers[i].position;
        obj["pts"] = drivers[i].points;
        obj["name"] = drivers[i].lastName;
    }

    serializeJson(doc, file);
    file.close();
    return true;
}

// ─── CONSTRUCTOR METHODS ──────────────────────────────────────────────────

bool DataCache::loadConstructors()
{
    if (!LittleFS.exists("/constructors.json"))
        return false;
    File file = LittleFS.open("/constructors.json", "r");
    if (!file)
        return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        file.close();
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    constructorCount = 0;
    for (JsonObject obj : arr)
    {
        if (constructorCount >= MAX_CONSTRUCTORS)
            break;

        constructors[constructorCount].position = obj["pos"] | 0;
        constructors[constructorCount].points = obj["pts"] | 0;
        strlcpy(constructors[constructorCount].name, obj["name"] | "", 64);
        constructorCount++;
    }

    file.close();
    return true;
}

bool DataCache::saveConstructors()
{
    File file = LittleFS.open("/constructors.json", "w");
    if (!file)
        return false;

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < constructorCount; i++)
    {
        JsonObject obj = arr.add<JsonObject>();
        obj["pos"] = constructors[i].position;
        obj["pts"] = constructors[i].points;
        obj["name"] = constructors[i].name;
    }

    serializeJson(doc, file);
    file.close();
    return true;
}