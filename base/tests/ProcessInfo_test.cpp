#include "base/ProcessInfo.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main()
{
  printf("pid = %d\n", Miren::base::ProcessInfo::pid());
  printf("uid = %d\n", Miren::base::ProcessInfo::uid());
  printf("euid = %d\n", Miren::base::ProcessInfo::euid());
  printf("start time = %s\n", Miren::base::ProcessInfo::startTime().toFormattedString().c_str());
  printf("hostname = %s\n", Miren::base::ProcessInfo::hostname().c_str());
  printf("opened files = %d\n", Miren::base::ProcessInfo::openedFiles());
  printf("threads = %zd\n", Miren::base::ProcessInfo::threads().size());
  printf("num threads = %d\n", Miren::base::ProcessInfo::numThreads());
  printf("status = %s\n", Miren::base::ProcessInfo::procStatus().c_str());
}
