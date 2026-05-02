#pragma once
#include <Arduino.h>
#include <time.h>

class TimeManager {
public:
    TimeManager();
    bool syncNTP();
    time_t getUTCTime() const;
    time_t getLocalTime() const;
    int getUTCOffset() const { return _utcOffset; }

private:
    int _utcOffset;
    time_t _baseUtcTime;
    unsigned long _baseMillis;
};
