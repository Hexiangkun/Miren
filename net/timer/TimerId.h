#pragma once

#include "base/Copyable.h"
#include <stdint.h>

namespace Miren
{
namespace net
{
    class Timer;
    // 它被设计用来取消Timer的，它的结构很简单，只有一个Timer指针和其序列号
    class TimerId : public base::Copyable
    {
    public:
        TimerId() :timer_(nullptr), sequence_(0) {}
        TimerId(Timer* timer, int64_t sequence) : timer_(timer), sequence_(sequence) {}

        friend class TimerQueue;    //TimerQueue为其友元，可以操作其私有数据
    private:
        Timer* timer_;              //一个Timer*指针
        int64_t sequence_;          //序列号
    };
    
} // namespace net
}