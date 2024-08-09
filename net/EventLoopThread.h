#pragma once

#include "base/Noncopyable.h"
#include "base/thread/Thread.h"
#include "base/thread/Mutex.h"
#include "base/thread/Condition.h"

namespace Miren
{
    namespace net
    {
        class EventLoop;

        class EventLoopThread : base::NonCopyable
        {
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
        private:
            void threadFunc();

        public:
            EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = std::string());
            ~EventLoopThread();

            EventLoop* startLoop();                     //启动线程，该线程就成为了IO线程 ，返回本线程中的EventLoop
        private:
            base::MutexLock mutex_;                     // 保护对loop对互斥操作
            base::Condition cond_ GUARDED_BY(mutex_);   //通知startLoop可以返回loop
            base::Thread thread_;                       //线程，线程内部执行EventLoop
            ThreadInitCallback callback_;               //线程初始化时的回调函数，执行一次

            EventLoop* loop_ GUARDED_BY(mutex_);        //本对象拥有的EventLoop指针
            bool exiting_;
        };
    } // namespace net
    
} // namespace Miren
