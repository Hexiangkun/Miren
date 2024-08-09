//
// Created by 37496 on 2024/6/24.
//
#include "base/log/AsyncLogging.h"
#include "base/log/LogFile.h"
#include "base/Timestamp.h"

namespace Miren::log
{
    AsyncLogging::AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval)
                    :flushInterval_(flushInterval), //设置超时时间
                    running_(false),
                    basename_(basename),
                    rollSize_(rollSize),
                    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"), //日志线程,设置线程启动执行的函数
                    latch_(1),
                    mutex_(),
                    cond_(mutex_),
                    currentBuffer_(new Buffer),
                    nextBuffer_(new Buffer),
                    buffers_()
    {
        currentBuffer_->bzero();
        nextBuffer_->bzero();
        buffers_.reserve(16);   //缓冲区列表预留16个空间
    }

    AsyncLogging::~AsyncLogging()
    {
        if(running_) {
            stop();
        }
    }

    //前端写日志消息进currentBuffer_缓冲并在写满时通知后端
    void AsyncLogging::append(const char* logline, int len)
    {
        base::MutexLockGuard lock(mutex_);
        if(currentBuffer_->avail() > len) {
            currentBuffer_->append(logline, len);   //当前currentBuffer_未满，直接追加日志数据到当前Buffer
        }
        else {
            //否则，说明当前缓冲已写满，就先把currentBuffer_记录的日志消息移入待写入文件的buffers_列表
            buffers_.push_back(std::move(currentBuffer_));

            if(nextBuffer_) {   //把预备好的另一块缓冲nextBuffer_移用为当前缓冲
                currentBuffer_ = std::move(nextBuffer_);
            }
            else {  //那就再分配一块新的Buffer(如果前端写入太快，导致current和next Buffer缓冲都用完了)
                currentBuffer_.reset(new Buffer);
            }
            currentBuffer_->append(logline, len);   //currentBuffer_弄好后，追加日志消息到currentBuffer_中
            cond_.notify();     //并通知（唤醒）后端开始写入日志数据
        }
    }



    void AsyncLogging::start()
    {
        running_ = true;
        thread_.start();    //日志写线程启动
        latch_.wait();      //等待线程已经启动，才继续往下执行
    }

    void AsyncLogging::stop() NO_THREAD_SAFETY_ANALYSIS
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }  

    //接收方后端线程把前端传来的日志写入到文件中
    void AsyncLogging::threadFunc()
    {
        assert(running_ == true);
        latch_.countDown();

        LogFile output(basename_, rollSize_, false, flushInterval_);

        BufferUPtr newBuffer1(new Buffer);
        BufferUPtr newBuffer2(new Buffer);
        newBuffer1->bzero();
        newBuffer2->bzero();

        BufferVector buffersToWrite;
        buffersToWrite.reserve(16);

        while (running_)
        {
            assert(newBuffer1 && newBuffer1->length() == 0);
            assert(newBuffer2 && newBuffer2->length() == 0);
            assert(buffersToWrite.empty());

            {
                base::MutexLockGuard lock(mutex_);
                //在临界区内等待条件触发。1.超时（flushInterval_） 2.前端写满了一个或多个buffer，见append
                if(buffers_.empty()) {
                    cond_.waitForSeconds(flushInterval_);
                }
                buffers_.push_back(std::move(currentBuffer_));
                currentBuffer_ = std::move(newBuffer1); //并立刻将空闲的newBuffer移为当前缓冲
                buffersToWrite.swap(buffers_);
                if(!nextBuffer_) {
                    nextBuffer_ = std::move(newBuffer2);
                }
            }

            assert(!buffersToWrite.empty());
            //消息堆积
            //前端陷入死循环，拼命发送日志消息，超过后端的处理能力，这就是典型的生产速度
            //超过消费速度的问题，会造成数据在内存中堆积，严重时引发性能问题（可用内存不足）
            //或程序崩溃（分配内存失败）
            if(buffersToWrite.size() > 25) {
                char buf[256];
                snprintf(buf, sizeof buf, "Dropped log message at %s, %zd larger buffers\n",
                base::Timestamp::now().toFormattedString().c_str(),
                buffersToWrite.size() - 2);
                fputs(buf, stderr);
                output.append(buf, static_cast<int>(strlen(buf)));
                buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
            }

            for(const auto& buffer : buffersToWrite) {
                output.append(buffer->data(), buffer->length());
            }

            if(buffersToWrite.size() > 2) {
                buffersToWrite.resize(2);   //仅保留两个buffer，用于newBuffer1 newBuffer2
            }

            if(!newBuffer1) {
                assert(!buffersToWrite.empty());
                newBuffer1 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer1->reset();
            }

            if(!newBuffer2) {
                assert(!buffersToWrite.empty());
                newBuffer2 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer2->reset();
            }

            buffersToWrite.clear();
            output.flush();
        }
        output.flush();
        
    }
}