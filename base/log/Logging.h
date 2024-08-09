#pragma once

#include "base/log/LogStream.h"
#include "base/Timestamp.h"

namespace Miren
{
namespace base
{
    class TimeZone;
}

namespace log
{
    //简单的日志记录分析，搭配AsyncLogging使用
    class Logger
    {
    public:
        //枚举类型定义了日志等级
        enum LogLevel
        {
            TRACE,//指出比DEBUG粒度更细的一些信息事件（开发过程中使用）
            DEBUG,//指出细粒度信息事件对调试应用程序是非常有帮助的。（开发过程中使用）
            INFO,//表明消息在粗粒度级别上突出强调应用程序的运行过程
            WARN,//系统能正常运行，但可能会出现潜在错误的情形
            ERROR,//指出虽然发生错误事件，但仍然不影响系统的继续运行
            FATAL, //指出每个严重的错误事件将会导致应用程序的退出
            NUM_LOG_LEVELS,//级别个数
        };

        //用于保存文件名和长度
        class SourceFile
        {
        public:
            const char* data_;
            int size_;

            template<int N>
            SourceFile(const char (&arr)[N]) : data_(arr), size_(N-1)
            {
                const char* slash = strrchr(data_, '/');    // builtin function 从data_的右侧开始查找字符/首次出现的位置
                if(slash) {
                    data_ = slash + 1;  //即只要源文件名,如/home/.../test.txt,只要test.txt
                    size_ -= static_cast<int>(data_ - arr);
                }
            }

            explicit SourceFile(const char* filename) : data_(filename)
            {
                const char* slash = strrchr(data_, '/');
                if(slash) {
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }
        };

        //各个构造函数中使用Impl类来生成日志消息格式
        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char* func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();  //注意，是在析构函数中，把消息输出到stdout屏幕上的！

        LogStream& stream() { return impl_.stream_; }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);

        typedef void (*OutputFunc)(const char* msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc);  //设置输出函数
        static void setFlush(FlushFunc);    //清空缓冲
        static void setTimeZone(const base::TimeZone& tz);

    private:
        //logger类内部的一个嵌套类，封装了Logger的缓冲区stream_,指定了生成日志消息的格式
        //具体实现。如一条日志消息：20180115 06:39:01.712150Z  7070 INFO  pid = 7070 - main.cc:13
        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
            void formatTime();      //格式化time_时间戳为年月日后存于stream_中
            void finish();          //将日志写到缓冲区

            base::Timestamp time_;  //时间
            LogStream stream_;      //构造日志缓冲区，该缓冲区重载了各种<<，都是将数据格式到LogStream的内部成员缓冲区buffer里
            LogLevel level_;        //日志等级
            int line_;              //行号(记录日志时用于显示代码的行号)
            SourceFile basename_;   //文件基本名称（记录日志时，用于显示文件的名称）
        };
        Impl impl_;                 //logger中通过构造这个对象生成日志消息的格式
    };

    extern Logger::LogLevel global_logLevel;

    inline Logger::LogLevel Logger::logLevel()
    {
        return global_logLevel;
    }
}

    namespace log
    {
        //__FILE__用以指示本行语句所在源文件的文件名
        //__LINE__用以指示本行语句在源文件中的位置信息
        //__func__指示当前的函数名
        #define LOG_TRACE if (Miren::log::Logger::logLevel() <= Miren::log::Logger::TRACE) \
            Miren::log::Logger(__FILE__, __LINE__, Miren::log::Logger::TRACE, __func__).stream()
        #define LOG_DEBUG if (Miren::log::Logger::logLevel() <= Miren::log::Logger::DEBUG) \
            Miren::log::Logger(__FILE__, __LINE__, Miren::log::Logger::DEBUG, __func__).stream()
        #define LOG_INFO if (Miren::log::Logger::logLevel() <= Miren::log::Logger::INFO) \
            Miren::log::Logger(__FILE__, __LINE__).stream()
        #define LOG_WARN Miren::log::Logger(__FILE__, __LINE__, Miren::log::Logger::WARN).stream()
        #define LOG_ERROR Miren::log::Logger(__FILE__, __LINE__, Miren::log::Logger::ERROR).stream()
        #define LOG_FATAL Miren::log::Logger(__FILE__, __LINE__, Miren::log::Logger::FATAL).stream()
        #define LOG_SYSERR Miren::log::Logger(__FILE__, __LINE__, false).stream()
        #define LOG_SYSFATAL Miren::log::Logger(__FILE__, __LINE__, true).stream()
    }

    #define CHECK_NOTNULL(val) \
    ::Miren::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

    // A small helper for CHECK_NOTNULL().
    template <typename T>
    T* CheckNotNull(log::Logger::SourceFile file, int line, const char *names, T* ptr)
    {
        if (ptr == nullptr)
        {
            log::Logger(file, line, log::Logger::FATAL).stream() << names;
        }
        return ptr;
    }
}