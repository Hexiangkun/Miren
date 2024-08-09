#pragma once

#include <functional>
#include <memory>
#include <pthread.h>
#include "base/Noncopyable.h"
#include "base/thread/CountDownLatch.h"
#include "base/thread/Atomic.h"

namespace Miren
{
namespace base
{
    class Thread : public NonCopyable
    {
    public:
        typedef std::function<void()> ThreadFunc;

        explicit Thread(ThreadFunc, const std::string& name = "");
        ~Thread();

        void start();   //启动线程
        int join();

        bool started() const { return started_; }
        bool joined() const { return joined_; }
        pid_t tid() const { return tid_; }
        const std::string name() const { return name_; }

        static int numCreated() { return numCreated_.get(); } //静态成员函数操作静态变量

    private:
        void setDefaultName();  //设置线程默认名字为Thread+序号(在Thread构造函数中使用)
    private:
        bool started_;  //是否已经启动线程
        bool joined_;   //是否调用了pthread_join()函数

        pthread_t pthreadId_;   //线程ID，（与其他进程中的线程ID可能相同）
        pid_t tid_;             //线程的真实pid，唯一
        ThreadFunc func_;       //线程要回调的函数
        std::string name_;      //线程名称(若不指定，默认为：Thread+序号)
        CountDownLatch latch_;

        static AtomicInt32 numCreated_; //原子性的静态成员。已经创建的线程个数,和其他线程个数有关，因此要static
    };
}
}