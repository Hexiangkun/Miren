#include "base/thread/Thread.h"
#include "base/Exception.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
namespace Miren
{
namespace base
{

namespace detail
{
    //通过系统调用获取tid
    pid_t gettid() {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }
    //fork之后的子进程要执行的函数
    void afterFork() {
        CurrentThread::t_cachedThreadId = 0;
        CurrentThread::t_threadName = "main";
        CurrentThread::tid();   //缓存线程tid
    }

    //主线程名的初始化
    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer()
        {
            CurrentThread::t_threadName = "main";   //设置主线程名main
            CurrentThread::tid();                   //缓存main线程tid
            pthread_atfork(nullptr, nullptr, &afterFork);
        }
    };
    //全局变量，在命名空间中，引用库的时候初始化，因此在main函数执行前则会初始化了！
    ThreadNameInitializer init;

    //线程数据类，观察者模式，通过回调函数传递给子线程的数据
    struct ThreadData
    {
        typedef Thread::ThreadFunc ThreadFunc;
        ThreadFunc func_;   //线程函数
        std::string name_;  //线程名称
        pid_t* tid_;        //线程pid
        CountDownLatch* latch_;

        ThreadData(ThreadFunc func, const std::string& name, pid_t* tid, CountDownLatch* latch) :
            func_(std::move(func)), name_(name), tid_(tid), latch_(latch)
        {

        }
        //ThreadData具体的执行
        void runInThread()
        {
            *tid_ = CurrentThread::tid();   //计算本线程id，保存在线程局部变量里
            tid_ = nullptr;
            latch_->countDown();
            latch_ = nullptr;

            CurrentThread::t_threadName = name_.empty() ? "MirenThread" : name_.c_str();
            ::prctl(PR_SET_NAME, CurrentThread::t_threadName);//为线程指定名字

            try{
                func_();    //运行线程函数
                CurrentThread::t_threadName = "finished";   //运行结束后的threadname
            }
            catch (const Exception& e) { //捕获线程函数是否有异常
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", e.what());
                fprintf(stderr, "stack trace: %s\n", e.stackTrace());
                abort();
            }
            catch (const std::exception& e) {
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", e.what());
                abort();
            }
            catch (...) {
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                throw ;
            }
        }
    };
    //父线程交给系统的回调，obj是父线程传递的threadData
    void* startThread(void* obj)
    {
        ThreadData* data = static_cast<ThreadData*>(obj);
        data->runInThread();
        delete data;
        return nullptr;
    }
}

//缓存tid到t_cachedTid
void CurrentThread::cacheTid() {
    if(t_cachedThreadId == 0) {
        t_cachedThreadId = detail::gettid();
        t_cachedThreadIdStrLength = snprintf(t_cachedThreadIdString, sizeof t_cachedThreadIdString, "%5d ", t_cachedThreadId);
    }
}

bool CurrentThread::isMainThread() {
    return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec) {
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t >(usec / 1000000);
    ts.tv_nsec = static_cast<long >(usec % 1000000 * 1000);
    ::nanosleep(&ts, nullptr);
}

//因为numCreated_是静态的，一定要定义，不能只在头文件中声明
AtomicInt32 Thread::numCreated_;

    Thread::Thread(ThreadFunc func, const std::string& name) 
                : 
                started_(false), 
                joined_(false),
                pthreadId_(0),
                tid_(0),
                func_(std::move(func)),
                name_(name),
                latch_(1)
    {
        setDefaultName();
    }

    Thread::~Thread() 
    {
        if(started_ && !joined_) {
            pthread_detach(pthreadId_);//析构时，如果没有调用join,则与线程分离
        }
    }
    //设置线程默认名字为Thread+序号
    void Thread::setDefaultName()
    {
        int num = numCreated_.get();
        if(name_.empty()) {
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", num);
            name_ = buf;
        }
    }

    int Thread::join() {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        return pthread_join(pthreadId_, nullptr);//阻塞等待pthreadId_线程结束,使得Thread对象的生命期长于线程
    }
    //启动线程
    void Thread::start() {
        assert(!started_);
        started_ = true;
        detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
        if(pthread_create(&pthreadId_, nullptr, &detail::startThread, data)) {//创建线程并启动
            started_ = false;
            delete data;
        }
        else {
            latch_.wait();
            assert(tid_ > 0);
        }
    }
    
} // namespace base

}