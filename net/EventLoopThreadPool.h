#pragma once

#include "base/Noncopyable.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
namespace Miren
{
    namespace net
    {
        class EventLoop;
        class EventLoopThread;

        class EventLoopThreadPool : base::NonCopyable
        {
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
            EventLoopThreadPool(EventLoop* baseLoop, const std::string& name);
            ~EventLoopThreadPool();

            void setThreadNum(int numThreads) { numThreads_ = numThreads; }
            void start(const ThreadInitCallback& cb = ThreadInitCallback());

            EventLoop* getNextLoop();
            EventLoop* getLoopForHash(size_t hashCode);

            std::vector<EventLoop*> getAllLoops();

            bool started() const { return started_; }
            const std::string& name() const { return name_; }

        private:
            EventLoop* baseLoop_;       // master Reactor线程，构造时从外部接受
            std::string name_;          // 线程池的名称
            bool started_;              // 是否开启线程池
            int numThreads_;            // 线程数目
            int next_;                  // 下一个线程id
            std::vector<std::unique_ptr<EventLoopThread>> threads_;
            std::vector<EventLoop*> loops_;
        };
    } // namespace net
    
} // namespace Miren
