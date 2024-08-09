#pragma once

#include "base/Noncopyable.h"
#include "base/thread/Mutex.h"
#include "base/thread/Condition.h"
#include <deque>
#include <assert.h>

namespace Miren
{
namespace base
{
    // 异步队列，底层使用std::deque来实现，没有大小限制
    template<typename T>
    class BlockingQueue : public NonCopyable
    {
    public:
        BlockingQueue() : mutex_(), notEmpty_(mutex_), queue_()
        {

        }


        void put(const T& x) 
        {
            MutexLockGuard lock(mutex_);
            queue_.push_back(x);
            notEmpty_.notify();
        }

        void put(T&& x) 
        {
            MutexLockGuard lock(mutex_);
            queue_.push_back(std::move(x));
            notEmpty_.notify();
        }

        T take()
        {
            MutexLockGuard lock(mutex_);
            while(queue_.empty()) {
                notEmpty_.wait();
            }
            assert(!queue_.empty());
            T front(std::move(queue_.front()));
            queue_.pop_front();
            return front;
        }

        size_t size() const
        {
            MutexLockGuard lock(mutex_);
            return queue_.size();
        }
    private:
        mutable MutexLock mutex_;
        Condition notEmpty_ GUARDED_BY(mutex_);
        std::deque<T> queue_ GUARDED_BY(mutex_);
    };
}
}