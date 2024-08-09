//
// Created by 37496 on 2024/6/17.
//

#include "base/log/LogFile.h"
#include "base/FileUtil.h"
#include "base/ProcessInfo.h"
namespace Miren::log
{
    LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe, int flushInterval, int checkEveryN)
                :basename_(basename), rollSize_(rollSize),
                flushInterval_(flushInterval), 
                checkEveryN_(checkEveryN),
                count_(0), 
                mutex_(threadSafe ? new base::MutexLock() : nullptr),//不是线程安全就不需要构造mutex_
                startOfPeriod_(0),
                lastRoll_(0),
                lastFlush_(0)
    {
        assert(basename.find('/') == std::string::npos);    //断言basename不包含'/'
        rollFile();
    }

    LogFile::~LogFile() = default;

    void LogFile::append(const char *logline, int len) {
        if(mutex_) {    //如果new过锁了，说明需要线程安全，那么调用加锁方式
            base::MutexLockGuard lock(*mutex_);
            append_unlock(logline, len);
        }
        else {
            append_unlock(logline, len);
        }
    }

    void LogFile::flush() {
        if(mutex_) {
            base::MutexLockGuard lock(*mutex_);
            file_->flush();
        }
        else {
            file_->flush();
        }
    }

    void LogFile::append_unlock(const char* logline, int len) {
        file_->append(logline, len);            //FileUtil.h中的AppendFile的不加锁apeend方法
        if(file_->writtenBytes() > rollSize_) { //写入字节数大小超过滚动设定大小
            rollFile();                         //滚动文件
        }
        else {
            ++count_;                   //增加行数

            if(count_ >= checkEveryN_) {
                count_ = 0;
                time_t now = ::time(nullptr);
                time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
                if(thisPeriod_ != startOfPeriod_) {
                    rollFile();
                }
                else if(now - lastFlush_ > flushInterval_) {
                    lastFlush_ = now;
                    file_->flush();
                }
            }
        }
    }

    bool LogFile::rollFile() {
        time_t now = 0;
        std::string filename = getLogFileName(basename_, &now); //生成一个包含时间的字符串名作为文件名
        //注意，这里先除KRollPerSeconds然后乘KPollPerSeconds表示对齐值KRollPerSeconds的整数倍，也就是事件调整到当天零点(/除法会引发取整)
        time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
        //如果now大于lastRoll，产生一个新的日志文件，并更新lastRoll
        if(now > lastRoll_) {
            lastRoll_ = now;
            lastFlush_ = now;
            startOfPeriod_ = start;
            file_.reset(new base::FileUtil::AppendFile(filename));
            return true;
        }
        return false;
    }

//获取生成一个文件名称
//传来的basename:logfile_test
//则生成文件名：logfile_test.20130411-115604.popo.7743.log（运行程序.时间.主机名.线程名.log）
    std::string LogFile::getLogFileName(const std::string& basename, time_t* now) {
        std::string filename;
        filename.reserve(basename.size() + 64); //预分配内存
        filename = basename;

        char timebuf[32];
        struct tm tm;
        *now = time(nullptr);
        gmtime_r(now, &tm);
        strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
        filename += timebuf;    //拼接上时间
        //拼接上主机名,根据processinfo模块内部调用::gethostname函数获取主机名
        filename += base::ProcessInfo::hostname();
        
        char pidbuf[32];
        snprintf(pidbuf, sizeof pidbuf, ".%d", base::ProcessInfo::pid());
        filename += pidbuf; //拼接上进程ID

        filename += ".log"; //拼接上日志标记.log
        return filename;
    }
}