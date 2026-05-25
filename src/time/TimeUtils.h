#pragma once
#include <time.h>

static inline time_t my_timegm(struct tm *t)
{
    return t->tm_sec + t->tm_min*60 + t->tm_hour*3600 + t->tm_yday*86400 +
           (t->tm_year-70)*31536000 + ((t->tm_year-69)/4)*86400 -
           ((t->tm_year-1)/100)*86400 + ((t->tm_year+299)/400)*86400;
}
