//
// Created by 37496 on 2024/6/16.
//

#ifndef SERVER_PROCESSINFO_H
#define SERVER_PROCESSINFO_H

#include "base/Timestamp.h"
#include "base/StringUtil.h"
#include <sys/types.h>
#include <string>
#include <vector>

namespace Miren
{
namespace base
{
//进程信息
namespace ProcessInfo
{
    pid_t pid();
    std::string pidString();
    uid_t uid();
    std::string username();
    uid_t euid();

    base::Timestamp startTime();
    int clockTicksPerSecond();
    int pageSize();
    bool isDebugBuild();

    std::string hostname();
    std::string procname();
    base::StringPiece procname(const std::string& stat);

    std::string procStatus();
    std::string procStat();
    std::string threadStat();
    std::string execPath();

    int openedFiles();
    int maxOpenFiles();

    struct CpuTime
    {
        double userSeconds;
        double systemSeconds;

        CpuTime() : userSeconds(0), systemSeconds(0) {}
    };

    CpuTime cpuTime();

    std::vector<pid_t > threads();

    int numThreads();
}
}
}


#endif //SERVER_PROCESSINFO_H
