#include "net/EventLoopThreadPool.h"
#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
namespace Miren
{
    namespace net
    {
        EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
            :baseLoop_(baseLoop),
            name_(name),
            started_(false),
            numThreads_(0),
            next_(0)    // getNextLoop使用
        {

        }

        EventLoopThreadPool::~EventLoopThreadPool()
        {
            // 不需要析构loop，因为它们都是栈上对象

        }
        // 启动线程池
        //使用之前,先执行setThreadNum()函数指定线程个数
        void EventLoopThreadPool::start(const ThreadInitCallback& cb)
        {
            assert(!started_);
            // 必须在baseLoop线程内开启线程池
            baseLoop_->assertInLoopThread();

            started_ = true;

            for(int i=0; i<numThreads_; i++) {
                char buf[name_.size() + 32];
                snprintf(buf, sizeof buf, "%s_%d", name_.c_str(), i);
                EventLoopThread* t = new EventLoopThread(cb, buf);
                threads_.push_back(std::unique_ptr<EventLoopThread>(t));
                loops_.push_back(t->startLoop());// startLoop()会创建并返回运行的EventLoop,然后push_back到loops_
            }
            //未指定线程个数,即只有一个EventLoop，则在这个EventLoop进入事件循环之前，调用cb回调
            if(numThreads_ == 0 && cb) {
                cb(baseLoop_);
            }
        }

        // master线程向线程池内分配任务
        EventLoop* EventLoopThreadPool::getNextLoop()
        {
            // 必须在base线程内执行
            baseLoop_->assertInLoopThread();
            assert(started_);
            EventLoop* loop = baseLoop_;

            if(!loops_.empty()) {
                loop = loops_[next_];
                ++next_;

                if(base::implicit_cast<size_t>(next_) >= loops_.size()) {
                    next_ = 0;
                }
            }
            return loop;
        }

        EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
        {
            baseLoop_->assertInLoopThread();
            assert(started_);
            EventLoop* loop = baseLoop_;

            if(!loops_.empty()) {
                loop = loops_[hashCode % loops_.size()];
            }
            return loop;
        }

        std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
        {
            baseLoop_->assertInLoopThread();
            assert(started_);
            if(loops_.empty()) {
                return std::vector<EventLoop*>(1, baseLoop_);
            }
            else {
                return loops_;
            }
        }
    } // namespace net
    
} // namespace Miren
