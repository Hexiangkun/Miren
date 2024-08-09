#include "net/timer/Timer.h"

namespace Miren::net
{
    base::AtomicInt64 Timer::s_numCreated_;

    void Timer::restart(base::Timestamp now)
    {
        if(repeat_) {
            expiration_ = base::addTime(now, interval_);//如果设置重复，则重新计算下一个超时时刻
        }
        else {
            expiration_ = base::Timestamp::invalid();   //如果不是重复定时，则另下一个时刻为非法时间即可
        }
    }
}