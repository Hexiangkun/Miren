#pragma once

#include "base/thread/Mutex.h"
#include "base/thread/CurrentThread.h"
#include "net/timer/TimerId.h"
#include "net/Callbacks.h"
#include <functional>
#include <vector>
#include <atomic>
#include <any>
#include <map>
namespace Miren
{
    namespace net
    {
        class Channel;
        class Poller;
        class TimerQueue;
        /*
        每个Reactor内部调用一个EventLoop
        内部不停进行poll或epoll_wait的调用，然后根据fd返回的事件，调用对应回调进行处理
        */
        class EventLoop
        {
        public:
            typedef std::function<void()> Functor;

            EventLoop();
            ~EventLoop();
            // 事件循环，不能跨线程调用
            void loop();
            // 这个函数可以跨线程调用，停止事件循环
            void quit();

            base::Timestamp pollReturnTime() const { return pollReturnTime_; }//poll返回的时间戳
            int64_t iteration() const { return iteration_; }
 /// 在它的IO线程内执行某个用户任务回调,避免线程不安全的问题，保证不会被多个线程同时访问
  /// 用来将非io线程内的任务放到pendingFunctors_中并唤醒wakeupChannel_事件来执行任务(在主循环中运行)
            void runInLoop(Functor cb);
            //将任务放到pendingFunctors_队列中并通过evnetfd唤醒IO线程执行任务
            void queueInLoop(Functor cb);
//返回任务队列pendingFunctors_大小
            size_t queueSize() const;

            TimerId runAt(base::Timestamp time, TimerCallback cb);  //某个时间点执行定时回调
            TimerId runAfter(double delay, TimerCallback cb);       //某个时间点之后执行定时回调
            TimerId runEvery(double interval, TimerCallback cb);    //在每个时间间隔处理某个回调事件
            void cancel(TimerId timerId);                           //删除timerId对应的定时器

            void wakeup();                          //写8个字节给eventfd，唤醒事件通知描述符。否则EventLoop::loop()的poll会阻塞
            void updateChannel(Channel* channel);   //在poller中注册或者更新通道
            void removeChannel(Channel* channel);   //从poller中移除通道
            bool hasChannel(Channel* channel);
            // 断言处于当前线程中（主要是因为有些接口不能跨线程调用），如果不是，则终止程序
            void assertInLoopThread();
            // EventLoop构造时会记录线程id，比较该pid和当前线程id就可以判断是否跨线程操作
            bool isInLoopThread() const { return threadId_ == base::CurrentThread::tid(); }
            bool eventHandling() const { return eventHanding_; }    //是否正在处理事件

            //用户自定义保存上下文,boost::any任何类型的数据都可以
            void setContext(const std::string& name, const std::any& context) { contexts_[name] = context; }
            const std::any& getContext(const std::string& name) const { return contexts_.at(name); }
            std::any* getMutableContext(const std::string& name) { return &contexts_.at(name); }
            void deleteContext(const std::string& name) { contexts_.erase(name); }

            static EventLoop* getEventLoopOfCurrentThread();//返回当前线程的EventLoop对象指针(__thread类型)

        private:
            void abortNotInLoopThread();    //不在IO线程,则退出程序
            void handleRead();              // wake up，将eventfd里的内容读走，以便让其继续检测事件通知
            void doPendingFunctors();       //执行pendingFunctors_中的任务

            void printActiveChannels() const;   //DEBUG
        private:
            typedef std::vector<Channel*> ChannelList;
            bool looping_;                      //是否处于事件循环
            std::atomic<bool> quit_;            //是否退出loop
            bool eventHanding_;                 //当前是否处于事件处理的状态

            int64_t iteration_;                 //poll返回的次数
            const pid_t threadId_;              //EventLoop构造函数会记住本对象所属的线程ID
            base::Timestamp pollReturnTime_;    //poll返回的时间戳

            std::unique_ptr<Poller> poller_;    //EventLoop首先一定得有个I/O复用才行,它的所有职责都是建立在I/O复用之上的
            std::unique_ptr<TimerQueue> timerQueue_;    //应该支持定时事件，关于定时器的所有操作和组织定义都在类TimerQueue中 

            int wakeupFd_;                              //用于eventfd的通知机制的文件描述符
            std::unique_ptr<Channel> wakeupChannel_;    //wakeupFd_对于的通道。若此事件发生便会一次执行pendingFunctors_中的可调用对象

            std::map<std::string, std::any> contexts_;  //用来存储用户想要保存的信息，std::any任何类型的数据都可以
            
            ChannelList activeChannels_;                //保存的是poller类中的poll调用返回的所有活跃事件集
            Channel* currentActiveChannel_;             //当前正在处理的活动通道

            mutable base::MutexLock mutex_;
            bool callingPendingFunctors_;
            std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
        };
    } // namespace net
    
} // namespace Miren
