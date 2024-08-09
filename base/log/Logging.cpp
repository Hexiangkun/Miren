#include "base/log/Logging.h"
#include "base/TimeZone.h"
#include "base/ErrorInfo.h"
#include "base/thread/CurrentThread.h"
#include <stdlib.h>
#include <assert.h>
namespace Miren
{
    namespace log
    {
        __thread char t_time[64];
        __thread time_t t_lastSecond;


        Logger::LogLevel initLogLevel()
        {
            if(::getenv("MIREN_LOG_TRACE")) {
                return Logger::TRACE;
            }
            else if(::getenv("MIREN_LOG_DEBUG")) {
                return Logger::DEBUG;
            }
            else {
                return Logger::INFO;
            }
        }

        Logger::LogLevel global_logLevel = initLogLevel();


        const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
            "TRACE ",
            "DEBUG ",
            "INFO  ",
            "WARN  ",
            "ERROR ",
            "FATAL ",
        };

        class T
        {
        public:
            const char* str_;
            const unsigned len_;

            T(const char* str, unsigned len) : str_(str), len_(len)
            {
                assert(strlen(str) == len_);
            }
        };

        inline LogStream& operator<<(LogStream& s, T v) {
            s.append(v.str_, v.len_);
            return s;
        }

        inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
            s.append(v.data_, v.size_);
            return s;
        }

        void defaultFlush()
        {
            fflush(stdout);
        }

        void defaultOutput(const char* msg, int len)
        {
            size_t n = fwrite(msg, 1, len, stdout);
            (void)n;
        }

        Logger::OutputFunc global_output = defaultOutput;
        Logger::FlushFunc global_flush = defaultFlush;

        base::TimeZone global_logTimeZone;
    } // namespace log 
    
    namespace log
    {
        Logger::Logger(SourceFile file, int line):impl_(INFO, 0, file, line)
        {

        }
        Logger::Logger(SourceFile file, int line, LogLevel level) : impl_(level, 0, file, line)
        {

        }
        Logger::Logger(SourceFile file, int line, LogLevel level, const char* func): impl_(level, 0, file, line)
        {
            impl_.stream_ << func << ' ';
        }
        Logger::Logger(SourceFile file, int line, bool toAbort) : impl_(toAbort ? FATAL : ERROR, errno, file, line)
        {

        }
        Logger::~Logger()
        {
            impl_.finish();
            const LogStream::Buffer& buf(stream().buffer());
            global_output(buf.data(), buf.length());
            if(impl_.level_ == FATAL) {
                global_flush();
                abort();
            }
        }

        void Logger::setLogLevel(Logger::LogLevel level)
        {
            global_logLevel = level;
        }

        void Logger::setOutput(OutputFunc out)
        {
            global_output = out;
        }

        void Logger::setFlush(FlushFunc flush)
        {
            global_flush = flush;
        }

        void Logger::setTimeZone(const base::TimeZone& tz)
        {
            global_logTimeZone = tz;
        }

    }

    namespace log
    {
        Logger::Impl::Impl(LogLevel level, int old_errno, const SourceFile& file, int line)
                :time_(base::Timestamp::now()),
                stream_(),
                level_(level),
                line_(line),
                basename_(file)
        {
            formatTime();
            base::CurrentThread::tid();
            stream_ << T(base::CurrentThread::tidString(), base::CurrentThread::tidStringLength());
            stream_ << T(LogLevelName[level], 6);
            if(old_errno != 0) {
                stream_ << base::ErrorInfo::strerror_tl(old_errno) << " (errno=" << old_errno << ") ";
            }
        }

        void Logger::Impl::formatTime()
        {
            int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
            time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / base::Timestamp::kMicroSecondsPerSecond);
            int microseconds = static_cast<int>(microSecondsSinceEpoch % base::Timestamp::kMicroSecondsPerSecond);
            if(seconds != t_lastSecond) {
                t_lastSecond = seconds;
                struct tm tm_time;
                if(global_logTimeZone.valid()) {
                    tm_time = global_logTimeZone.toLocalTime(seconds);
                }
                else {
                    ::gmtime_r(&seconds, &tm_time);
                }

                int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                                    tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                                    tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
                assert(len == 17); (void)len;
            }
            if(global_logTimeZone.valid()) {
                Fmt us(".%06d ", microseconds);
                assert(us.length() == 8);
                stream_ << T(t_time, 17) << T(us.data(), 8);
            }
            else {
                Fmt us(".%06dZ ", microseconds);
                assert(us.length() == 9);
                stream_ << T(t_time, 17) << T(us.data(), 9);
            }
        }

        void Logger::Impl::finish()
        {
            stream_ << " - " << basename_ << ':' << line_ << '\n';
        }
    }
}   