#include "base/log/LogFile.h"
#include "base/log/Logging.h"
#include <unistd.h>
#include <string.h>
#include <memory>

std::unique_ptr<Miren::log::LogFile> g_logFile;

void outputFunc(const char* msg, int len)
{
    g_logFile->append(msg, len);
}

void flushFunc()
{
    g_logFile->flush();
}

int main()
{
    char name[256] = "logfile";

    g_logFile.reset(new Miren::log::LogFile(name, 200*1000));
    Miren::log::Logger::setOutput(outputFunc);
    Miren::log::Logger::setFlush(flushFunc);

    std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    for(int i=0; i<10000; i++) {
        LOG_INFO << line << i;
        usleep(1000);
    }
}