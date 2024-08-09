//
// Created by 37496 on 2024/6/17.
//

#ifndef SERVER_LOGFILE_H
#define SERVER_LOGFILE_H

#include "base/Noncopyable.h"
#include "base/thread/Mutex.h"
#include <string>
#include <memory>

namespace Miren
{
namespace base::FileUtil
{
    class AppendFile;
}

namespace log
{
    //用于把日志记录到文件的类
    //一个典型的日志文件名：logfile_test.20130411-115604.popo.7743.log
    //运行程序.时间.主机名.线程名.log
    class LogFile : base::NonCopyable
    {
    public:
        LogFile(const std::string& basename,
                off_t rollSize,
                bool threadSafe = true,
                int flushInterval = 3,
                int checkEveryN = 1024);//默认分割行数1024
        ~LogFile();

        void append(const char* logline, int len);//将一行长度为len添加到日志文件中
        void flush();       //刷新
        bool rollFile();    //滚动文件

    private:
        void append_unlock(const char* logline, int len);   //不加锁的append方式
        //生成日志文件的名称(运行程序.时间.主机名.线程名.log)
        static std::string getLogFileName(const std::string& base, time_t* now);
    private:
        const std::string basename_;    //日志文件名前面部分
        const off_t rollSize_;          // 日志文件达到rollSize_换一个新文件
        const int flushInterval_;       // 日志写入文件刷新间隔时间
        const int checkEveryN_;

        int count_;                     //计数行数，检测是否需要换新文件（count_>checkEveryN_也会滚动文件）
        std::unique_ptr<base::MutexLock> mutex_;
        time_t startOfPeriod_;          // 开始记录日志时间（调整到零时时间, 时间/ kRollPerSeconds_ * kRollPerSeconds_）
        time_t lastRoll_;               // 上一次滚动日志文件时间
        time_t lastFlush_;              // 上一次日志写入文件时间
        std::unique_ptr<base::FileUtil::AppendFile> file_;

        const static int kRollPerSeconds_ = 60 * 60 * 24;   //一天的时间
    };
}
}


#endif //SERVER_LOGFILE_H
