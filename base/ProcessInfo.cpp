//
// Created by 37496 on 2024/6/16.
//

#include "base/ProcessInfo.h"
#include "base/FileUtil.h"
#include "base/thread/CurrentThread.h"
#include <dirent.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <assert.h>

namespace Miren::base
{
    namespace detail
    {
        __thread int t_numOpenedFiles = 0;
        int fdDirFilter(const struct dirent* d)
        {
            if(::isdigit(d->d_name[0])) {
                ++t_numOpenedFiles;
            }
            return 0;
        }

        __thread std::vector<pid_t >* t_pids = nullptr;
        int taskDirFilter(const struct dirent* d) {
            if(::isdigit(d->d_name[0])) {
                t_pids->push_back(atoi(d->d_name));
            }
            return 0;
        }


        int scanDir(const char* dirpath, int (*filter)(const struct dirent*)) {
            struct dirent** namelist = nullptr;
            int result = ::scandir(dirpath, &namelist, filter, alphasort);
            assert(namelist == nullptr);
            return result;
        }

        base::Timestamp g_startTime = base::Timestamp::now();

        int g_clockTicks = static_cast<int >(::sysconf(_SC_CLK_TCK));
        int g_pageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));
    }

    namespace ProcessInfo
    {
        pid_t pid()
        {
            return ::getpid();
        }

        std::string pidString()
        {
            char buf[32];
            snprintf(buf, sizeof buf, "%d", pid());
            return buf;
        }

        uid_t uid()
        {
            return ::getuid();
        }

        std::string username()
        {
            struct passwd pwd;
            struct passwd* result = nullptr;
            char buf[8192];
            const char* name = "unknownuser";

            getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
            if(result) {
                name = pwd.pw_name;
            }
            return name;
        }

        uid_t euid()
        {
            return ::geteuid();
        }

        base::Timestamp startTime()
        {
            return detail::g_startTime;
        }

        int clockTicksPerSecond()
        {
            return detail::g_clockTicks;
        }

        int pageSize()
        {
            return detail::g_pageSize;
        }

        bool isDebugBuild()
        {
#ifndef NODEBUG
            return false;
#else
            return true;
#endif
        }


        std::string hostname()
        {
            char buf[256];
            if(::gethostname(buf, sizeof buf) == 0) {
                buf[sizeof(buf) - 1] = '\0';
                return buf;
            } else{
                return "unknownhost";
            }
        }

        std::string procname()
        {
            return std::string(procname(procStat()));
        }

        base::StringPiece procname(const std::string& stat)
        {
            base::StringPiece name;
            size_t lp = stat.find('(');
            size_t rp = stat.find(')');
            if(lp != std::string::npos && rp != std::string::npos && lp < rp) {
                name = std::string_view(stat.data() + lp + 1, rp - lp - 1);
            }
            return name;
        }

        std::string procStatus()
        {
            std::string result;
            base::FileUtil::readFile("/proc/self/status", 65536, &result);
            return result;
        }

        std::string procStat()
        {
            std::string result;
            base::FileUtil::readFile("/proc/self/stat", 65536, &result);
            return result;
        }
        std::string threadStat()
        {
            char buf[64];
            snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", base::CurrentThread::tid());
            std::string result;
            base::FileUtil::readFile(buf, 65536, &result);
            return result;
        }

        std::string execPath()
        {
            std::string result;
            char buf[1024];
            ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
            if(n > 0) {
                result.assign(buf, n);
            }
            return result;
        }

        int openedFiles()
        {
            detail::t_numOpenedFiles = 0;
            detail::scanDir("/proc/self/fd", detail::fdDirFilter);
            return detail::t_numOpenedFiles;
        }

        int maxOpenFiles()
        {
            struct rlimit rl;
            if(::getrlimit(RLIMIT_NOFILE, &rl)) {
                return openedFiles();
            }
            else {
                return static_cast<int>(rl.rlim_cur);
            }
        }

        CpuTime cpuTime()
        {
            CpuTime t;
            struct tms tms;
            if(::times(&tms) >= 0) {
                const double hz = static_cast<double >(clockTicksPerSecond());
                t.userSeconds = static_cast<double >(tms.tms_utime) / hz;
                t.systemSeconds = static_cast<double >(tms.tms_stime) / hz;
            }
            return t;
        }

        std::vector<pid_t > threads()
        {
            std::vector<pid_t > result;
            detail::t_pids = &result;
            detail::scanDir("/proc/self/task", detail::taskDirFilter);
            detail::t_pids = nullptr;
            std::sort(result.begin(), result.end());
            return result;
        }

        int numThreads()
        {
            int result = 0;
            std::string status = procStatus();
            size_t pos = status.find("Threads:");
            if(pos != std::string::npos) {
                result = ::atoi(status.c_str() + pos + 8);
            }
            return result;
        }



    }

}
