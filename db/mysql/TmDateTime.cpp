//
// Created by 37496 on 2024/3/1.
//

#include "TmDateTime.h"
#include <assert.h>
#include <cmath>
#include <cstring>


namespace SqlConn
{
    ////////////////////////////////////////////////
    TmDateTime::TmDateTime(int year, int month, int day, int hour, int minute, int second, int millisecond, int microsecond, int naonsecond)
    {
        assert(year > 1708);
        assert(year < 2292);

        double jd = ToJulianDay(year, month, day) - 2451544.5;

        ticks = ((int64_t)jd) * 86400000000000LL + hour * 3600000000000LL + minute * 60000000000LL + second * 1000000000LL + millisecond * 1000000LL + microsecond * 1000LL + naonsecond;
    }

    double TmDateTime::ToJulianDay(int year, int month, int day, int hour, int minute, int second, int millisecond, int microsecond)
    {
        // lookup table for (153*month - 457)/5 - note that 3 <= month <= 14.
        static int lookup[] = { -91, -60, -30, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275, 306, 337 };

        // day to double
        double dday = double(day) + ((double((hour * 60 + minute) * 60 + second) * 1000 + millisecond) * 1000 + microsecond) / 86400000000.0;
        if (month < 3)
        {
            month += 12;
            --year;
        }
        double dyear = double(year);
        return dday + lookup[month] + 365 * year + std::floor(dyear / 4) - std::floor(dyear / 100) + std::floor(dyear / 400) + 1721118.5;
    }

    void TmDateTime::checkLimit(int& lower, int& higher, int limit)
    {
        if (lower >= limit)
        {
            higher += int(lower / limit);
            lower = int(lower % limit);
        }
    }

    int TmDateTime::daysOfMonth(int year, int month) const
    {
        assert(month >= 1 && month <= 12);

        static int daysOfMonthTable[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        if (month == 2 && isLeapYear(year))
            return 29;
        else
            return daysOfMonthTable[month];
    }

    tm TmDateTime::ToTm() const
    {
        double julianDate = ToJulianDate();

        double z = std::floor(julianDate - 1721118.5);
        double r = julianDate - 1721118.5 - z;
        double g = z - 0.25;
        double a = std::floor(g / 36524.25);
        double b = a - std::floor(a / 4);
        int _year = int(std::floor((b + g) / 365.25));
        double c = b + z - std::floor(365.25*_year);
        int _month = int(std::floor((5 * c + 456) / 153));
        double dday = c - std::floor((153.0*_month - 457) / 5) + r;
        int _day = int(dday);
        if (_month > 12)
        {
            ++_year;
            _month -= 12;
        }

        if (_day > daysOfMonth(_year, _month))
        {
            _day -= daysOfMonth(_year, _month);
            if (++_month > 12)
            {
                ++_year;
                _month -= 12;
            }
        }

        int doy = 0;
        for (int month = 1; month < _month; ++month)
            doy += daysOfMonth(_year, month);
        doy += _day;

        int second = (int)((ticks % 60000000000LL) / 1000000000LL);
        int minute = (int)((ticks % 3600000000000LL) / 60000000000LL);
        int hour = (int)((ticks % 86400000000000LL) / 3600000000000LL);

#ifdef _WIN32
        return ::tm{ second, minute, hour, _day, _month - 1, _year - 1900, int((std::floor(julianDate + 1.5))) % 7, doy - 1, -1 };
#else
        return ::tm{ second, minute, hour, _day, _month - 1, _year - 1900, int((std::floor(julianDate + 1.5))) % 7, doy - 1, -1, 0, 0 };
#endif

    }

    std::string TmDateTime::ToString() const
    {
        if (ticks == INT64_MAX) return "Unknown";
        if (ticks == INT64_MIN) return "unknowN";
        tm filtm = ToTm();
        char buf[160];
        int64_t mls = (std::abs(ticks) % 1000000000LL) / 1000000;

#ifdef _WIN32
#pragma warning (disable:4996)
#endif
        sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d.%03ld", filtm.tm_year + 1900, filtm.tm_mon + 1, filtm.tm_mday, filtm.tm_hour, filtm.tm_min, filtm.tm_sec, mls);
#ifdef _WIN32
#pragma warning (default:4996)
#endif

        return std::string(buf);
    }

    bool TmDateTime::Parse(const std::string & str, TmDateTime * timptr)
    {
        if (str == "Unknown")
        {
            timptr->ticks = INT64_MAX;
            return true;
        }
        if (str == "unknowN")
        {
            timptr->ticks = INT64_MIN;
            return true;
        }

        tm filtm;
        std::memset(&filtm, 0, sizeof(tm));
        int mls = 0;
#ifdef _WIN32
#pragma warning (disable:4996)
#endif
        int numflds = sscanf(str.c_str(), "%d-%d-%dT%d:%d:%d.%d", &filtm.tm_year, &filtm.tm_mon, &filtm.tm_mday, &filtm.tm_hour, &filtm.tm_min, &filtm.tm_sec, &mls);
#ifdef _WIN32
#pragma warning (default:4996)
#endif
        if (numflds < 1) return false;
        if (filtm.tm_mon == 0) filtm.tm_mon = 1;
        if (filtm.tm_mday == 0) filtm.tm_mday = 1;

        if ((filtm.tm_year < 1700) || (filtm.tm_year > 2300) ||
            (filtm.tm_mon < 1) || (filtm.tm_mon > 12) ||
            (filtm.tm_mday < 1) || (filtm.tm_mday > 31)) return false;

        timptr->ticks = SqlConn::TmDateTime(filtm.tm_year, filtm.tm_mon, filtm.tm_mday, filtm.tm_hour, filtm.tm_min, filtm.tm_sec, mls).ticks;

        return true;

    }

    const TmDateTime TmDateTime::MaxValue = TmDateTime(INT64_MAX);
    const TmDateTime TmDateTime::MinValue = TmDateTime(INT64_MIN);
    const TmDateTime TmDateTime::Year1900 = TmDateTime(1900, 1, 1);
    const TmDateTime TmDateTime::Year1958 = TmDateTime(1958, 1, 1);
    const TmDateTime TmDateTime::Year1970 = TmDateTime(1970, 1, 1);
}

