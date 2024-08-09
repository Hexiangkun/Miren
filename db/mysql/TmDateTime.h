#pragma once

#include <string>
#include <ctime>
#include <sys/stat.h>
#include <stdexcept>

namespace SqlConn
{
    /////////////////////////////////////////////
    //Poco::DateTime dt(2000, 1, 1).utcTime() 100us resolution ->  131659776000000000
    //.NET DateTime(2000, 1, 1).Ticks  100us resolution ->         630822816000000000
    // 946674000 sec 1970->2000

    // valid: 22.09.1707 - 10.04.2292

    class TmDateTime
    {

    protected:
        int64_t ticks;			//

    private:
        void checkLimit(int& lower, int& higher, int limit);
        inline bool isLeapYear(int year) const
        {
            return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
        }

        int daysOfMonth(int year, int month) const;

        inline double ToJulianDate() const {
            double utcDays = (double)(ticks / 86400000000000LL);
            return utcDays + 2451544.5;
        }

    public:

        static const TmDateTime MaxValue;
        static const TmDateTime MinValue;
        static const TmDateTime Year1900;
        static const TmDateTime Year1958;
        static const TmDateTime Year1970;

        TmDateTime()
                :ticks(0) {}

        TmDateTime(const TmDateTime &other)
        {
            ticks = other.ticks;
        }

        TmDateTime& operator=(const TmDateTime& other)
        {
            ticks = other.ticks;
            return *this;
        }

        TmDateTime(int64_t nanoSeconds)
                :ticks(nanoSeconds) {}

        TmDateTime(
                int year,
                int month,
                int day,
                int hour = 0,
                int minute = 0,
                int second = 0,
                int millisecond = 0,
                int microsecond = 0,
                int naonsecond = 0)
        ;

        static TmDateTime FileWriteTime(const std::string &path)
        {
#ifdef _WIN32
#define _stat_ _stat32i64
#else
#define _stat_ stat
#endif
            struct _stat_ fileInfo;

            if (_stat_(path.c_str(), &fileInfo) != 0)
            {
                throw std::runtime_error("Failed to get last write time.");
            }
            return FromStdTime(fileInfo.st_mtime);
        }

        static TmDateTime FromStdTime(std::time_t stdt) {
            return TmDateTime((stdt - 946674000) * 1000000000LL);
        }

        static TmDateTime Now() {
            return FromStdTime(time(NULL));
        }

        tm ToTm() const;
        std::string ToString() const;
        static bool Parse(const std::string &str, TmDateTime *timptr);

        static double ToJulianDay(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0, int microsecond = 0);


        inline bool operator < (const TmDateTime &other) const { return ticks < other.ticks; }
        inline bool operator > (const TmDateTime &other) const { return ticks > other.ticks; }
        inline bool operator >= (const TmDateTime &other) const { return ticks >= other.ticks; }
        inline bool operator <= (const TmDateTime &other) const { return ticks <= other.ticks; }
        inline bool operator == (const TmDateTime &other) const { return ticks == other.ticks; }
        inline bool operator != (const TmDateTime &other) const { return ticks != other.ticks; }

        inline TmDateTime operator + (const TmDateTime &other) const { return TmDateTime(ticks + other.ticks); }
        inline TmDateTime operator - (const TmDateTime &other) const { return TmDateTime(ticks - other.ticks); }


        inline int64_t Ticks() const { return ticks; }
        inline std::time_t epochTime() const { return std::time_t((ticks / 1000000000LL) + 946674000); }

        // .NET DateTime
        inline int64_t NetDateTimeTicks() const { return ticks / 100 + 630822816000000000LL; }
        //
// Summary:
//     Gets the date component of this instance.
//
// Returns:
//     A new object with the same date as this instance, and the time value set to 12:00:00
//     midnight (00:00:00).
        inline TmDateTime Date() const {
            return TmDateTime(ticks - (ticks % 86400000000000LL));
        }

        inline TmDateTime AddDays(long double value) { return AddHours(value * 24.L); }
        inline TmDateTime AddHours(long double value) { return AddMinutes(value * 60.L); }
        inline TmDateTime AddMinutes(long double value) { return AddNanoseconds(value *	 60000000000.L); }
        inline TmDateTime AddSeconds(long double value) { return AddNanoseconds(value *   1000000000.L); }
        inline TmDateTime AddMilliseconds(long double value) { return AddNanoseconds(value * 1000000.L); }
        inline TmDateTime AddMicroseconds(long double value) { return AddNanoseconds(value *    1000.L); }
        inline TmDateTime AddNanoseconds(long double value) { return TmDateTime(ticks + (int64_t)value); }
    };
}


