//
// Created by 37496 on 2024/6/11.
//

#ifndef SERVER_CONDITION_H
#define SERVER_CONDITION_H

#include "base/thread/Mutex.h"
#include <pthread.h>

namespace Miren
{
namespace base
{
    // pthread_cond条件变量的封装，与Mutex.h一起使用
    class Condition : public NonCopyable
    {
    public:
        explicit Condition(MutexLock& mutex) : mutex_(mutex)
        { 
            MCHECK(pthread_cond_init(&pcond_, nullptr)); //初始化条件变量
        }

        ~Condition() {
            MCHECK(pthread_cond_destroy(&pcond_));      //销毁
        }

        void wait() {
            MutexLock::UnassignGuard ug(mutex_);        //这里先给mutex_的holder_置零。在等其析构时会给holder_赋值
            MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));//阻塞等待条件，直到notify唤醒。原子操作。
        }

        bool waitForSeconds(double seconds);//设置等待条件变量超时seconds时间

        //唤醒一个等待条件变量的线程
        void notify() {
            MCHECK(pthread_cond_signal(&pcond_));
        }
        //唤醒所有等待条件变量的线程
        void notifyAll() {
            MCHECK(pthread_cond_broadcast(&pcond_));
        }

    private:
        MutexLock& mutex_;      //引用互斥量，不负责其生命期
        pthread_cond_t pcond_;  //条件变量
    };
} // namespace base
}


#endif //SERVER_CONDITION_H
