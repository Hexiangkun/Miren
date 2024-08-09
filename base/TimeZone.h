//
// Created by 37496 on 2024/6/24.
//

#ifndef SERVER_TIMEZONE_H
#define SERVER_TIMEZONE_H


#include "base/Copyable.h"
#include <memory>
#include <time.h>

namespace Miren
{
namespace base
{
    // TimeZone for 1970~2030 时区与夏令时
    class TimeZone : public base::Copyable
    {
    public:
        struct Data;

    public:
        explicit TimeZone(const char* zonefile);
        TimeZone(int eastOfUtc, const char* tzname);
        TimeZone() = default;

        bool valid() const { return static_cast<bool>(data_); }

        struct tm toLocalTime(time_t secondsSinceEpoch) const;
        time_t fromLocalTime(const struct tm&) const;


        static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false);

        static time_t fromUtcTime(const struct tm&);

        static time_t fromUtcTime(int year, int month, int day, int hour, int minute, int seconds);
    private:
        std::shared_ptr<Data> data_;
    };
    
} // namespace base
}


#endif //SERVER_TIMEZONE_H
