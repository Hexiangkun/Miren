#include "base/log/Logging.h"
#include "base/log/LogFile.h"
#include "base/thread/ThreadPool.h"
#include "base/TimeZone.h"

#include <stdio.h>
#include <memory>
#include <unistd.h>
#include <sys/syscall.h>

int g_total;
FILE* g_file;
std::unique_ptr<Miren::log::LogFile> g_logFile;

void dummyOutput(const char* msg, int len)
{
  g_total += len;
  if (g_file)
  {
    fwrite(msg, 1, len, g_file);
  }
  else if (g_logFile)
  {
    g_logFile->append(msg, len);
  }
}

void bench(const char* type)
{
  Miren::log::Logger::setOutput(dummyOutput);
  Miren::base::Timestamp start(Miren::base::Timestamp::now());
  g_total = 0;

  int n = 1000*1000;
  const bool kLongLog = false;
  std::string empty = " ";
  std::string longStr(3000, 'X');
  longStr += " ";
  for (int i = 0; i < n; ++i)
  {
    LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
             << (kLongLog ? longStr : empty)
             << i;
  }
  Miren::base::Timestamp end(Miren::base::Timestamp::now());
  double seconds = timeDifference(end, start);
  printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
         type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
  LOG_INFO << "logInThread";
  usleep(1000);
}

int main()
{
  getppid(); // for ltrace and strace

  Miren::base::ThreadPool pool("pool");
  pool.start(5);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);
  pool.run(logInThread);

  LOG_TRACE << "trace";
  LOG_DEBUG << "debug";
  LOG_INFO << "Hello";
  LOG_WARN << "World";
  LOG_ERROR << "Error";
  LOG_INFO << sizeof(Miren::log::Logger);
  LOG_INFO << sizeof(Miren::log::LogStream);
  LOG_INFO << sizeof(Miren::log::Fmt);
  LOG_INFO << sizeof(Miren::log::LogStream::Buffer);

  sleep(1);
  bench("nop");

  char buffer[64*1024];

  g_file = fopen("/dev/null", "w");
  setbuffer(g_file, buffer, sizeof buffer);
  bench("/dev/null");
  fclose(g_file);

  g_file = fopen("/tmp/log", "w");
  setbuffer(g_file, buffer, sizeof buffer);
  bench("/tmp/log");
  fclose(g_file);

  g_file = NULL;
  g_logFile.reset(new Miren::log::LogFile("test_log_st", 500*1000*1000, false));
  bench("test_log_st");

  g_logFile.reset(new Miren::log::LogFile("test_log_mt", 500*1000*1000, true));
  bench("test_log_mt");
  g_logFile.reset();

  {
  g_file = stdout;
  sleep(1);
  Miren::base::TimeZone beijing(8*3600, "CST");
  Miren::log::Logger::setTimeZone(beijing);
  LOG_TRACE << "trace CST";
  LOG_DEBUG << "debug CST";
  LOG_INFO << "Hello CST";
  LOG_WARN << "World CST";
  LOG_ERROR << "Error CST";

  sleep(1);
  Miren::base::TimeZone newyork("/usr/share/zoneinfo/America/New_York");
  Miren::log::Logger::setTimeZone(newyork);
  LOG_TRACE << "trace NYT";
  LOG_DEBUG << "debug NYT";
  LOG_INFO << "Hello NYT";
  LOG_WARN << "World NYT";
  LOG_ERROR << "Error NYT";
  g_file = NULL;
  }
  bench("timezone nop");
}
