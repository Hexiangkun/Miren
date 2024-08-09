#include "net/timer/TimerQueue.h"
#include "net/timer/Timer.h"
#include "net/timer/TimerId.h"
#include "net/EventLoop.h"
#include "base/log/Logging.h"
#include <sys/timerfd.h>
#include <unistd.h>

namespace Miren::net
{

    namespace detail
    {
        int createTimerfd() //创建非阻塞timerfd
        {
            int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
            if(timerfd < 0) {
                LOG_SYSFATAL << "Failed in timerfd_create.";
            }
            return timerfd;
        }

        //现在距离超时时间when还有多久
        struct timespec howMuchTimeFromNow(base::Timestamp when)
        {
            int64_t microseconds = when.microSecondsSinceEpoch() - base::Timestamp::now().microSecondsSinceEpoch();
            if(microseconds < 100) {
                microseconds = 100;
            }
            struct timespec ts;
            ts.tv_sec = static_cast<time_t>(microseconds / base::Timestamp::kMicroSecondsPerSecond);
            ts.tv_nsec = static_cast<long>(microseconds % base::Timestamp::kMicroSecondsPerSecond);
            return ts;
        }

        void readTimerfd(int timerfd, base::Timestamp now)  //超时后，timerfd变为可读
        {
            uint64_t howmany;
            ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
            LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
            if(n != sizeof howmany) {
                LOG_ERROR << "TimerQueue::handleRead() reads" << n << " bytes instead of 8";
            }
        }
        //重新设置timerfd表示的定时器超时时间,并启动
        void resetTimerfd(int timerfd, base::Timestamp expiration)
        {
            struct itimerspec newValue;
            struct itimerspec oldValue;
            base::MemoryZero(&newValue, sizeof newValue);
            base::MemoryZero(&oldValue, sizeof oldValue);
            newValue.it_value = howMuchTimeFromNow(expiration);             //计算和now之间的超时时间差
            int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);  //设置新定时器并开始计时
            if(ret) {
                LOG_SYSERR << "timerfd_settime()";
            }
        }
    }
    TimerQueue::TimerQueue(EventLoop* loop)
                :loop_(loop),
                timerfd_(detail::createTimerfd()),
                timerfdChannel_(loop, timerfd_),    //该channel负责timerfd分发事件
                timers_(),      //初始定时器集合空
                callingExpiredTimers_(false)
    {
        //设置定时器超时返回可读事件时要执行的回调函数，读timerfd
        timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
        //timerfd对应的channel监听事件为可读事件
        timerfdChannel_.enableReading();
    }

    TimerQueue::~TimerQueue()
    {
        timerfdChannel_.disableAll();
        timerfdChannel_.remove();
        ::close(timerfd_);
        for(const Entry& timer : timers_) {
            delete timer.second;    //释放Timer*
        }
    }

    TimerId TimerQueue::addTimer(TimerCallback cb, base::Timestamp when, double interval)
    {
        // 首先创建一个Timer对象，然后将cb放在里面。内部有一个run函数，调用的就是cb
        Timer* timer = new Timer(std::move(cb), when, interval);
        // 然后将这个timer丢到eventLoop里面去执行
        loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
        return TimerId(timer, timer->sequence());
    }

    void TimerQueue::cancel(TimerId timerId)
    {
        loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
    }

    void TimerQueue::addTimerInLoop(Timer* timer)
    {
        loop_->assertInLoopThread();
        bool earliestChanged = insert(timer);    //插入一个定时器，有可能会使得最早到期的定时器发生改变（插入的时间更小时）
        if(earliestChanged) {   //要插入的timer是最早超时的定时器
            detail::resetTimerfd(timerfd_, timer->expiration());    //重置timerfd的超时时刻
        }
    }

    void TimerQueue::cancelInLoop(TimerId timerId)
    {
        loop_->assertInLoopThread();
        //相等的。这两个容器保存的是相同的数据，timers_是按到期时间排序，activeTimers_按对象地址排序
        assert(timers_.size() == activeTimers_.size());
        //获取TimerId的Timer*和其序列号给timer
        ActiveTimer timer(timerId.timer_, timerId.sequence_);
        //寻找要取消的timer是否在activeTimers_中
        ActiveTimerSet::iterator it = activeTimers_.find(timer);
        if(it != activeTimers_.end()) {    //要取消的在当前激活的Timer集合中
            size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
            assert(n == 1); (void)n;        // Entry:    typedef std::pair<Timestamp, Timer*> Entry;
            delete it->first;               //timers_:   std::set<Entry>
            activeTimers_.erase(it);        //在activeTimers_中取消
        }
        else if(callingExpiredTimers_) {    //如果不在定时器集合中,而是属于在执行超时回调的定时器，则加入到cancelingTimers集合中
            cancelingTimers_.insert(timer);//在reset函数中不会再被重启
        }
        assert(timers_.size() == activeTimers_.size());
    }
    //处理timerfd读事件，执行超时函数
    void TimerQueue::handleRead()      // called when timerfd_ alarms
    {
        loop_->assertInLoopThread();
        base::Timestamp now(base::Timestamp::now());
        detail::readTimerfd(timerfd_, now); //必须要读取timerfd，否则会一直返回就绪事件

        std::vector<Entry> expired = getExpired(now);   //找到now之前所有超时的定时器列表

        callingExpiredTimers_ = true;   //设置正在处理超时事件的标志位
        cancelingTimers_.clear();
        for(const Entry& it : expired) {
            it.second->run();    //对所有超时的定时器expired执行对应的超时回调函数
        }
        callingExpiredTimers_ = false;

        reset(expired, now);    //把要重复运行的定时器重新加入到定时器集合中
    }
    //找到now之前的所有超时的定时器列表
    //并把超时的定时器从集合中删除
    std::vector<TimerQueue::Entry> TimerQueue::getExpired(base::Timestamp now)
    {
        assert(timers_.size() == activeTimers_.size());
        std::vector<Entry> expired;         //用于存放超时的定时器集合
        Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
        TimerList::iterator end = timers_.lower_bound(sentry);    //返回第一个大于等于now时间的迭代器，小于now的都已经超时
        assert(end == timers_.end() || now < end->first);
        std::copy(timers_.begin(), end, std::back_inserter(expired));   //[begin end)之间的元素，即达到超时的定时器追加到expired末尾
        timers_.erase(timers_.begin(), end);    //从timers_集合中删除到期的定时器

        for(const Entry& it : expired) {
            ActiveTimer timer(it.second, it.second->sequence());
            size_t n = activeTimers_.erase(timer);  //从activeTimers_中删除到期的定时器
            assert(n == 1); (void)n;
        }
        assert(timers_.size() == activeTimers_.size());
        return expired; //返回达到超时的定时器列表
    }

    //把要重复设置的定时器重新加入到定时器中
    void TimerQueue::reset(const std::vector<Entry>& expired, base::Timestamp now)
    {
        base::Timestamp nextExpire;

        for(const Entry& it : expired) {
            ActiveTimer timer(it.second, it.second->sequence());
            //设置了重复定时且不在cancelingTimers_(要取消的定时器)集合中
            if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
                it.second->restart(now);    //重新设置定时器时间
                insert(it.second);  //重新插入到定时器集合timers_和activeTimers中
            }
            else {//一次性定时器，不能被重置的，直接删除
                delete it.second;
            }
        }

        if(!timers_.empty()) {
            nextExpire = timers_.begin()->second->expiration();//取出第一个超时时间
        }

        if(nextExpire.valid()) {    //重新设置timefd的超时时刻
            detail::resetTimerfd(timerfd_, nextExpire);
        }
    }
    //插入一个timer到集合中,并返回最早到期时间是否改变
    bool TimerQueue::insert(Timer* timer)
    {
        loop_->assertInLoopThread();
        assert(timers_.size() == activeTimers_.size());
        bool earliestChanged = false;       //最早到期时间是否改变
        base::Timestamp when = timer->expiration();
        TimerList::iterator it = timers_.begin();
        //比较当前要插入的定时器是否时最早到时的
        //1.起初没有定时器 2.比当前timers_最早的定时器早
        if(it == timers_.end() || when < it->first) {
            earliestChanged = true;
        }
        {   
            //插入定时器到timers集合中,按照Timestamp排序
            std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
            assert(result.second); (void)result;
        }
        {
            //插入activeTimers_定时器集合中,按照Timer*地址排序
            std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
            assert(result.second); (void)result;
        }
        assert(timers_.size() == activeTimers_.size());
        return earliestChanged; //返回最早到期时间是否改变bool

    }
        
}