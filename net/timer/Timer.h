#pragma once

#include "base/Noncopyable.h"
#include "base/Timestamp.h"
#include "base/thread/Atomic.h"
#include "net/Callbacks.h"

namespace Miren
{
namespace net
{
    //Timer封装了定时器的一些参数，例如超时回调函数、超时时间、定时器是否重复、重复间隔时间、定时器的序列号。
    //其函数大都是设置这些参数，run()用来调用回调函数，restart()用来重启定时器（如果设置为重复）。 
    class Timer : base::NonCopyable
    {
    public:
        Timer(TimerCallback cb, base::Timestamp when, double interval)
            :callback_(std::move(cb)),  //回调函数
            expiration_(when),          //超时时间
            interval_(interval),        //如果重复，间隔时间
            repeat_(interval > 0.0),    //是否重复
            sequence_(s_numCreated_.incrementAndGet())  //设置当前定时器序列号，原子操作,先加后获取
            {

            } 

            //超时时调用的回调函数
            void run() const
            {
                callback_();
            }

            //返回定时器的闹铃时间
            base::Timestamp expiration() const { return expiration_; }
            //是否重复设置定时器
            bool repeat() const { return repeat_; }
            //定时器序列号
            int64_t sequence() const { return sequence_; }
            //重新设置定时器
            void restart(base::Timestamp now);

            static int64_t numCreated() { return s_numCreated_.get(); }

    private:
        const TimerCallback callback_;  //回调函数
        base::Timestamp expiration_;    //超时时间（绝对时间）
        const double interval_;         //间隔多久重新闹铃
        const bool repeat_;             //是否重复定时
        const int64_t sequence_;        //Timer序号,从s_numCreated_获取

        static base::AtomicInt64 s_numCreated_; //Timer计数，当前已经创建的定时器数量
    };
    
} // namespace net
}