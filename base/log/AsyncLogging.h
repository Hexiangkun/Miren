#pragma once

#include "base/log/LogStream.h"
#include "base/Noncopyable.h"
#include "base/thread/Thread.h"
#include "base/thread/CountDownLatch.h"
#include "base/thread/Mutex.h"
#include "base/thread/Condition.h"
#include <vector>
#include <memory>
#include <atomic>

namespace Miren
{
namespace log
{
    /*
    muduo异步日志库采用的双缓冲技术：
    前端负责往Buffer A填日志消息，后端负责将Buffer B的日志消息写入文件。
    当Buffer A写满后，交换A和B，让后端将Buffer A的数据写入文件，而前端则往Buffer B
    填入新的日志消息，如此往复。
    */
    class AsyncLogging : public base::NonCopyable
    {
    public:
        AsyncLogging(const std::string& basename, off_t rollSize, 
                        int flushInterval = 3);//超时时间默认3s（在超时时间内没有写满，也要将缓冲区的数据添加到文件当中）
        ~AsyncLogging();

        void append(const char* logline, int len);

        void start();
        void stop() NO_THREAD_SAFETY_ANALYSIS;
        
    private:
        void threadFunc();  //供后端消费者线程调用（将数据写到日志文件）

    private:
        typedef Miren::log::detail::FixedBuffer<Miren::log::detail::kLargeBuffer> Buffer;   //固定一个Buffer大小为4M
        typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
        typedef BufferVector::value_type BufferUPtr;

        const int flushInterval_;       //刷新间隔，在超时时间内没有写满，也要将这块缓冲区的数据添加到文件当中
        std::atomic<bool> running_;     //线程是否执行的标志
        const std::string basename_;    //日志文件名称
        const off_t rollSize_;          //日志文件滚动大小，当超过一定大小，则滚动一个新的日志文件

        Miren::base::Thread thread_;    //使用了一个单独的线程来记录日志！
        Miren::base::CountDownLatch latch_; //用于等待线程启动
        Miren::base::MutexLock mutex_;
        Miren::base::Condition cond_ GUARDED_BY(mutex_);
        BufferUPtr currentBuffer_ GUARDED_BY(mutex_);   //当前缓冲
        BufferUPtr nextBuffer_ GUARDED_BY(mutex_);      //预备缓冲
        BufferVector buffers_ GUARDED_BY(mutex_);       //待写入文件的已填满的缓冲列表，供后端写入的buffer

    };
}
}