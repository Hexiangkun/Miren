#include "net/EventLoopThread.h"
#include "net/EventLoop.h"

namespace Miren
{   
    namespace net
    {
        
        EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
                :
                mutex_(),
                cond_(mutex_),
                thread_(std::bind(&EventLoopThread::threadFunc, this), name),   //创建线程对象，指定线程函数
                callback_(cb),
                loop_(nullptr),
                exiting_(false)
        {

        }
        EventLoopThread::~EventLoopThread()
        {
            exiting_ = true;
            if(loop_ != nullptr) {
                loop_->quit();
                thread_.join();
            }
        }

        // 开启线程，并返回持有EventLoop的指针
        EventLoop* EventLoopThread::startLoop()
        {
            assert(!thread_.started());
            thread_.start();    //IO线程启动，调用threadFunc()中会执行EventLoop.loop()

            EventLoop* loop = nullptr;
            {
                base::MutexLockGuard lock(mutex_);
                while(loop_ == nullptr) {
                    cond_.wait();   //等待threadFunc()创建好当前IO线程,notify()时则会返回
                }
                loop = loop_;
            }
            return loop;
        }

        //由另一个线程在thread_启动后调用的函数
        void EventLoopThread::threadFunc()
        {
            EventLoop loop;
            if(callback_) {
                callback_(&loop);
            }
            // 防止竞态条件，防止上面startLoop返回一个空指针
            // loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了  
            // 就意味着线程退出了，EventLoopThread对象也就没有存在的价值了
            // 因而不会有什么大的问题  
            {
                base::MutexLockGuard lock(mutex_);
                loop_ = &loop;
                cond_.notify();//创建好后通知startLoop()中被阻塞的线程
            }

            loop.loop();    //会在这里循环，直到EventLoopThread析构。此后不再使用loop_访问EventLoop了
            base::MutexLockGuard lock(mutex_);
            loop_ = nullptr;
        }

    } // namespace net
    
} // namespace Miren
