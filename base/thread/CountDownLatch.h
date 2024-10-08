#pragma once

#include "base/Noncopyable.h"
#include "base/thread/Mutex.h"
#include "base/thread/Condition.h"

namespace Miren
{
namespace base
{
//“倒计时门栓”同步类
//这个是参考Java的一个同步辅助类，在完成一组正在其他线程中执行的操作之前，它允许一个或多个线程一直等待
//例如一组线程等待一个命令，让命令到来时，这些线程才开始运行。或者一个线程等待多个线程运行结束后才可以运行
    class CountDownLatch : public NonCopyable
    {
    public:
        explicit CountDownLatch(int count);
        void wait();            //阻塞等待，实际是条件变量的使用
        void countDown();       //每次调用都会给计数器count_减1，当计数器为0，则发起通知等待的线程
        int getCount() const;   //获取count_
    private:
        mutable MutexLock mutex_;                    //mutable可变的，否则getCount() const无法使用上锁改变锁的状态
        Condition condition_ GUARDED_BY(mutex_);     //条件变量
        int count_ GUARDED_BY(mutex_);               //计数器，当这个数值为零时，才会通知阻塞在调用wait()的线程
    };
    
} // namespace base
}