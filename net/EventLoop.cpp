#include "net/EventLoop.h"
#include "net/Channel.h"
#include "net/poller/Poller.h"
#include "net/timer/TimerQueue.h"
#include "net/sockets/SocketsOps.h"
#include "base/log/Logging.h"
#include "base/thread/CurrentThread.h"
#include <sys/eventfd.h>
#include <signal.h>
#include <unistd.h>
namespace
{
// 屏蔽SIGPIPE信号
//忽略SIGPIPE信号，防止对方断开连接时继续写入造成服务进程意外退出
#pragma GCC diagnostic ignored "-Wold-style-cast"
    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
        }
    };
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj; //全局对象
}

namespace Miren
{
    namespace net
    {   
        namespace detail
        {
            //创建非阻塞eventfd
            int createEventfd()
            {
                int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
                if(evtfd < 0) {
                    LOG_SYSERR << "Failed in eventfd";
                    abort();
                }

                return evtfd;
            }
        } // namespace detail
        
        // 线程内部的全局变量，记录本线程持有EventLoop的指针
        __thread EventLoop* t_loopInThisThread = 0;
        // poll或epoll最大超时时间
        const int kPollTimeMs = 10000;

        //返回当前线程的EventLoop对象（one loop per thread）
        EventLoop* EventLoop::getEventLoopOfCurrentThread() 
        {
            return t_loopInThisThread;
        }

        EventLoop::EventLoop()
                    :looping_(false),
                    quit_(false),
                    eventHanding_(false),
                    iteration_(0),
                    threadId_(base::CurrentThread::tid()),
                    poller_(Poller::newDefaultPoller(this)),
                    timerQueue_(new TimerQueue(this)),//用于管理定时器
                    wakeupFd_(detail::createEventfd()),//创建eventfd，用于唤醒线程用(跨线程激活,使用eventfd线程之间的通知机制)
                    wakeupChannel_(new Channel(this, wakeupFd_)),
                    currentActiveChannel_(nullptr),
                    callingPendingFunctors_(false)
        {
            LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
            if(t_loopInThisThread) {//检查当前线程是否创建了EventLoop对象（one loop per thread）
                //如果当前线程，已经有了一个EventLoop对象，则LOG_FATAL终止程序。
                LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_; 
            }
            else {
                t_loopInThisThread = this;
            }
            //设置eventfd唤醒线程后的读回调函数
            wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
            ////开启eventfd的读事件
            wakeupChannel_->enableReading();
        }

        EventLoop::~EventLoop()
        {
            LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
                    << " destructs in thread " << base::CurrentThread::tid();
            wakeupChannel_->disableAll();
            wakeupChannel_->remove();
            ::close(wakeupFd_);
            t_loopInThisThread = nullptr;
        }

        void EventLoop::loop()
        {
            assert(!looping_);      //禁止重复开启loop
            assertInLoopThread();   // 禁止跨线程，断言在创建EventLoop对象的线程调用loop，因为loop函数不能跨线程调用
            looping_ = true;
            quit_ = false;
            LOG_TRACE << "EventLoop " << this << " start looping";

            while(!quit_) {
                activeChannels_.clear(); //首先清除上一次的活跃channel
                //使用epoll_wait等待事件到来，并把到来的事件填充至activeChannels
                pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
                ++iteration_;
                if(log::Logger::logLevel() <= log::Logger::TRACE) {
                    printActiveChannels();
                }

                //开始处理回调事件
                eventHanding_ = true;
                for(Channel* channel : activeChannels_) {
                    currentActiveChannel_ = channel;
                    currentActiveChannel_->handleEvent(pollReturnTime_);
                }
                currentActiveChannel_ = nullptr;
                eventHanding_ = false;
                // 执行pending Functors_中的任务回调
                // 这种设计使得IO线程也能执行一些计算任务，避免了IO线程在不忙时长期阻塞在IO multiplexing调用中
                doPendingFunctors();
            }
            LOG_TRACE << "EventLoop " << this << " stop looping";
            looping_ = false;
        }
        //结束循环.可跨线程调用
        void EventLoop::quit()
        {
            quit_ = true;
            if(!isInLoopThread()) {
                wakeup();//如果不是IO线程调用，则需要唤醒IO线程。因为此时IO线程可能正在阻塞或者正在处理事件
            }
        }
        //在它的IO线程内执行某个用户任务回调,避免线程不安全的问题，保证不会被多个线程同时访问。
        void EventLoop::runInLoop(Functor cb)
        {
            // 在本线程内，直接执行
            if(isInLoopThread()) {
                cb();
            }
            else {
                // 跨线程，放入队列
                //不是当前IO线程，则加入队列，等待IO线程被唤醒再调用
                queueInLoop(std::move(cb));
            }
        }
        //将任务放到pendingFunctors_队列中并通过evnetfd唤醒IO线程执行任务
        void EventLoop::queueInLoop(Functor cb)
        {
            // 把任务加入到队列可能同时被多个线程调用，需要加锁
            {
                base::MutexLockGuard lock(mutex_);
                pendingFunctors_.push_back(std::move(cb));
            }
            // 将cb放入队列后，我们还需要唤醒IO线程来及时执行Functor
            // 有两种情况：
            // 1.如果调用queueInLoop()的不是IO线程，需要唤醒,才能及时执行doPendingFunctors()
            // 2.如果在IO线程调用queueInLoop()，且此时正在调用pending functor(原因：
            //    防止doPendingFunctors()调用的Functors再次调用queueInLoop，
            //    循环回去到poll的时候需要被唤醒进而继续执行doPendingFunctors()，否则新增的cb可能不能及时被调用),
            // 即只有在IO线程的事件回调中调用queueInLoop()才无需唤醒(即在handleEvent()中调用queueInLoop ()不需要唤醒
            //    ，因为接下来马上就会执行doPendingFunctors())
            // 如果是跨线程或者正字啊处理之前的IO任务， 那么就需要向eventFd写入数据，唤醒epoll，执行任务队列里面的任务
            if(!isInLoopThread() || callingPendingFunctors_) {
                wakeup();//写一个字节来唤醒poll阻塞，触发wakeupFd可读事件
            }
        }

        size_t EventLoop::queueSize() const 
        {
            base::MutexLockGuard lock(mutex_);
            return pendingFunctors_.size();
        }

        TimerId EventLoop::runAt(base::Timestamp time, TimerCallback cb)
        {
            return timerQueue_->addTimer(std::move(cb), time, 0.0);
        }
        TimerId EventLoop::runAfter(double delay, TimerCallback cb)
        {
            base::Timestamp time(base::addTime(base::Timestamp::now(), delay));
            return runAt(time, std::move(cb));
        }

        TimerId EventLoop::runEvery(double interval, TimerCallback cb)
        {
            base::Timestamp time(base::addTime(base::Timestamp::now(), interval));
            return timerQueue_->addTimer(std::move(cb), time, interval);
        }

        void EventLoop::cancel(TimerId timerId)
        {
            timerQueue_->cancel(timerId);
        }

        // EventLoop持有eventfd包装的channel
        // 向eventfd写入数据，触发读事件
        void EventLoop::wakeup()
        {
            uint64_t one = 1;
            ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);

            if(n != sizeof one) {
                LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
            }
        }

        void EventLoop::updateChannel(Channel* channel)
        {
            assert(channel->ownerLoop() == this);
            assertInLoopThread();
            poller_->updateChannel(channel);
        }

        void EventLoop::removeChannel(Channel* channel)
        {
            assert(channel->ownerLoop() == this);
            assertInLoopThread();
            if(eventHanding_) {
                assert(currentActiveChannel_ == channel || 
                    std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
            }
            poller_->removeChannel(channel);
        }

        bool EventLoop::hasChannel(Channel* channel)
        {
            assert(channel->ownerLoop() == this);
            assertInLoopThread();
            return poller_->hasChannel(channel);
        }

        void EventLoop::assertInLoopThread()
        {
            if(!isInLoopThread()) {
                abortNotInLoopThread();
            }
        }

        void EventLoop::abortNotInLoopThread()
        {
            LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this 
                        << " was created int threadId_ = " << threadId_
                        << ", current thread id = " << base::CurrentThread::tid();
        }

        //eventFd读事件触发时的回调函数
        void EventLoop::handleRead()          // wake up
        {
            uint64_t one = 1;
            ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
            if(n != sizeof one) {
                LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
            }
        }

        //并发问题：runInLoop会跨线程，改变pendingFunctors_的内容
        void EventLoop::doPendingFunctors()
        {
            std::vector<Functor> functors;
            callingPendingFunctors_ = true;//设置标志位,表示当前在执行Functors任务
            //好！swap到局部变量中，再下面回调！
            //好处：１.缩减临界区长度,意味这不会阻塞其他线程调用queueInLoop
            //       2.避免死锁(因为Functor可能再调用queueInLoop)
            {
                base::MutexLockGuard lock(mutex_);
                functors.swap(pendingFunctors_);
            }

            for(const Functor& functor : functors) {
                functor();
            }
            callingPendingFunctors_ = false;
        }

        void EventLoop::printActiveChannels() const
        {
            for(const Channel* channel : activeChannels_) {
                LOG_TRACE << "{" << channel->reventsToString() << "} ";
            }
        }

    } // namespace net
    
} // namespace Miren
