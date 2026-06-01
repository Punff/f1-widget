#pragma once
#include <time.h>
#include <cstdio>

static inline time_t my_timegm(struct tm *t)
{
    int year = t->tm_year + 1900;
    int month = t->tm_mon + 1;
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    int days = 365 * year + year / 4 - year / 100 + year / 400;
    days += (153 * month - 457) / 5 + t->tm_mday - 306;
    return (time_t)(days - 719163) * 86400 + t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
}

// Format a time difference as "Xd Yh Zm", "Xh Ym", or "<1m"
static inline void formatCountdown(char *buf, size_t len, time_t diffSecs) {
    if (diffSecs < 0) diffSecs = 0;
    if (diffSecs < 60) {
        snprintf(buf, len, "<1m");
        return;
    }
    int days = diffSecs / 86400;
    int hours = (diffSecs % 86400) / 3600;
    int mins = (diffSecs % 3600) / 60;

    if (days > 0)
        snprintf(buf, len, "%dd %dh %dm", days, hours, mins);
    else if (hours > 0)
        snprintf(buf, len, "%dh %dm", hours, mins);
    else
        snprintf(buf, len, "%dm", mins);
}
