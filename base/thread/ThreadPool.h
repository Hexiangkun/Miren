//
// Created by 37496 on 2024/6/16.
//

#ifndef SERVER_THREADPOOL_H
#define SERVER_THREADPOOL_H

#include "base/Noncopyable.h"
#include "base/thread/Mutex.h"
#include "base/thread/Condition.h"
#include "base/thread/Thread.h"
#include <functional>
#include <vector>
#include <deque>
#include <memory>

namespace Miren
{
namespace base
{
    class ThreadPool : NonCopyable
    {
    public:
        typedef std::function<void()> Task;

        explicit ThreadPool(const std::string& nameArg = "ThreadPool");
        ~ThreadPool();

        void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
        void setThreadInitCallback(const Task& task) { threadInitCallback_ = task; }

        void start(int numThreads);//启动固定的线程数目的线程池
        void stop();

        const std::string& name() const { return name_; }
        size_t queueSize() const ;

        void run(Task f);//往线程池当中的队列添加任务

    private:
        bool isFull() const REQUIRES(mutex_);
        void runInThread();//线程池当中的线程要执行的函数
        Task take();//获取任务

    private:
        mutable MutexLock mutex_;
        Condition notEmpty_ GUARDED_BY(mutex_);//非空条件变量
        Condition notFull_ GUARDED_BY(mutex_);//未满条件变量
        std::string name_;//线程池名称
        Task threadInitCallback_;
        std::vector<std::unique_ptr<base::Thread>> threads_;    //线程池中的线程
        std::deque<Task > queue_ GUARDED_BY(mutex_);    
        size_t maxQueueSize_;//最大队列大小，若达到最大队列，则需要等待线程（消费者）取出队列
        bool running_;
    };
}
}

#endif //SERVER_THREADPOOL_H
