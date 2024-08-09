#pragma once

#include "base/Noncopyable.h"
#include "base/Timestamp.h"
#include "net/Callbacks.h"
#include "net/Channel.h"
#include <set>
#include <vector>

namespace Miren
{
namespace net
{
    class EventLoop;
    class Timer;
    class TimerId;
    ///虽然TimerQueue中有Queue，但是其实现是基于Set的，而不是Queue。
    ///这样可以高效地插入、删除定时器，且找到当前已经超时的定时器。TimerQueue的public接口只有两个，添加和删除。
    class TimerQueue : base::NonCopyable
    {
    private:
        typedef std::pair<base::Timestamp, Timer*> Entry;
        typedef std::set<Entry> TimerList;
        typedef std::pair<Timer*, int64_t> ActiveTimer;
        typedef std::set<ActiveTimer> ActiveTimerSet;

    private:
        EventLoop* loop_;                   // 所属的EventLoop
        const int timerfd_;                 //timefd加入epoll,超时可读
        Channel timerfdChannel_;            //用于观察timerfd_的readable事件（超时则可读）

        TimerList timers_;                  //定时器集合，按到期时间排序
        ActiveTimerSet activeTimers_;       //定时器集合,按照Timer* 地址大小来排序
        bool callingExpiredTimers_;         //是否正在处理超时定时事件
        ActiveTimerSet cancelingTimers_;    //保存要取消的定时器的集合(如果不在定时器集合中,而是属于在执行超时回调的定时器)
    public:
        explicit TimerQueue(EventLoop* loop);
        ~TimerQueue();
        //添加定时器，线程安全
        TimerId addTimer(TimerCallback cb, base::Timestamp when, double interval);
        //取消定时器，线程安全
        void cancel(TimerId timerId);

    private:
        //以下成员函数只可能在其所属IO线程中调用，因而不必加锁
        void addTimerInLoop(Timer* timer);
        void cancelInLoop(TimerId timerId);

        void handleRead();      //处理timerfd读事件，执行超时函数

        std::vector<Entry> getExpired(base::Timestamp now); //返回超时的定时器列表,并把超时的定时器从集合中删除
        void reset(const std::vector<Entry>& expired, base::Timestamp now); //把要重复运行的定时器重新加入到定时器集合中
        bool insert(Timer* timer);  //插入定时器
        

    };
    
} // namespace net
}