//
// Created by 37496 on 2024/6/16.
//

#include "base/thread/ThreadPool.h"
#include "base/Exception.h"

namespace Miren::base
{
    ThreadPool::ThreadPool(const std::string &nameArg)
        :mutex_(),
         notEmpty_(mutex_), notFull_(mutex_),
         name_(nameArg), maxQueueSize_(0),
         running_(false)
    {

    }

    ThreadPool::~ThreadPool()
    {
        if(running_) {
            stop();
        }
    }

    void ThreadPool::start(int numThreads) {
        assert(threads_.empty());
        running_ = true;
        threads_.reserve(numThreads);
        for(int i=0;i<numThreads;i++){
            char id[32];
            snprintf(id, sizeof id, "%d", i+1);
            threads_.emplace_back(new base::Thread(std::bind(&ThreadPool::runInThread, this), name_ + id));
            threads_[i]->start();
        }
        //如果线程池为空，且有回调函数，则调用回调函数。这时相当与只有一个主线程
        if(numThreads == 0 && threadInitCallback_) {
            threadInitCallback_();
        }
    }

    void ThreadPool::stop() {
        {
            MutexLockGuard lock(mutex_);
            running_ = false;
            notEmpty_.notifyAll();  //通知所有等待在任务队列上的线程
        }
        for(auto & uptr : threads_) {
            uptr->join();
        }
    }

    size_t ThreadPool::queueSize() const {
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }

    void ThreadPool::run(Miren::base::ThreadPool::Task task) {
        if(threads_.empty()) {  //如果发现线程池中的线程为空，则直接执行任务
            task();
        }
        else {//若池中有线程，则把任务添加到队列，并通知线程
            MutexLockGuard lock(mutex_);
            while (isFull()) {//当任务队列已满
                notFull_.wait();    //等待非满通知,再往下执行添加任务到队列
            }
            assert(!isFull());
            queue_.push_back(std::move(task));    //任务队列未满，则添加任务到队列
            notEmpty_.notify();                   //告知任务队列已经非空，可以执行了
        }
    }

    ThreadPool::Task ThreadPool::take() {
        MutexLockGuard lock(mutex_);    //任务队列需要保护
        while (queue_.empty() && running_) {    //等待队列不为空，即有任务
            notEmpty_.wait();
        }
        Task task;
        if(!queue_.empty()) {
            task = queue_.front();
            queue_.pop_front();
            if(maxQueueSize_ > 0) {//通知，告知任务队列已经非满了，可以放任务进来了
                notFull_.notify();
            }
        }
        return task;
    }
    //判断任务队列是否已满  
    bool ThreadPool::isFull() const {
        mutex_.assertLocked();
        return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
    }

    //线程要执行的函数（在start()函数中被绑定）
    void ThreadPool::runInThread() {
        try {
            if(threadInitCallback_) {   //如果有回调函数，先调用回调函数。为任务执行做准备
                threadInitCallback_();
            }
            while (running_) {
                Task task(take());  //取出任务。有可能阻塞在这里，因为任务队列为空
                if(task) {
                    task();//执行任务
                }
            }
        }
        catch (const Exception& ex) {
            fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            abort();
        }
        catch (const std::exception& ex) {
            fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        }
        catch (...) {
            fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
            throw; // rethrow
        }
    }
}