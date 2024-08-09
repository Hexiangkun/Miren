#include "base/thread/CountDownLatch.h"

namespace Miren
{
namespace base
{


    CountDownLatch::CountDownLatch(int count) : mutex_(), condition_(mutex_), count_(count)//传递的计数器
    {

    }

    //实际是条件变量的使用
    //阻塞等待,直到计算器减为0
    void CountDownLatch::wait() {
        MutexLockGuard lock(mutex_);
        while(count_ > 0) {
            condition_.wait();  //count_>0，则一直阻塞，直到count_=0后的condition_.notifyAll()执行
        }
    }
    //计数器减一
    void CountDownLatch::countDown() {
        MutexLockGuard lock(mutex_);
        --count_;
        if(count_ == 0) {
            condition_.notifyAll(); //当count_为零时，才会通知阻塞在调用wait()的线程
        }
    }

    //返回当前计数器值
    int CountDownLatch::getCount() const {
        MutexLockGuard lock(mutex_);
        return count_;
    }
    
} // namespace base
} // namespace Miren