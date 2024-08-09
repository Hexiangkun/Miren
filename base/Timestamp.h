//
// Created by 37496 on 2024/6/12.
//

#ifndef SERVER_TIMESTAMP_H
#define SERVER_TIMESTAMP_H

#include "base/Copyable.h"
#include "base/Types.h"

namespace Miren
{
namespace base
{

    /// UTC时间戳(us)（距离1970-01-01 00:00:00 ）
    class Timestamp : public Copyable
    {
    public:
        static const int kMicroSecondsPerSecond = 1000 * 1000;
        Timestamp() : microSecondsSinceEpoch_(0) {}

        explicit Timestamp(int64_t microSecondsSinceEpochArg) : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
        {

        }

        void swap(Timestamp& that) {
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

        std::string toString() const;
        std::string toFormattedString(bool showMicroSeconds = true) const;

        bool valid() const { return microSecondsSinceEpoch_ > 0; }

        int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
        time_t secondSinceEpoch() const {
            return static_cast<time_t >(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
        }

        static Timestamp now();
        static Timestamp invalid() {
            return Timestamp();
        }

        static Timestamp fromUnixTime(time_t t) {
            return fromUnixTime(t, 0);
        }

        static Timestamp fromUnixTime(time_t t, int microseconds) {
            return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
        }
    private:
        int64_t microSecondsSinceEpoch_;
    };

    inline bool operator<(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs) {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    inline double timeDifference(Timestamp high, Timestamp low) {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double >(diff) / Timestamp::kMicroSecondsPerSecond;
    }

    inline Timestamp addTime(Timestamp timeStamp, double seconds) {
        int64_t delta = static_cast<int64_t >(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timeStamp.microSecondsSinceEpoch() + delta);
    }
    
} // namespace base

}

#endif //SERVER_TIMESTAMP_H
