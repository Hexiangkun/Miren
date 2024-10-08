#include "base/FileUtil.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace Miren::base;

int main()
{
  std::string result;
  int64_t size = 0;
  int err = FileUtil::readFile("/proc/self", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/proc/self", 1024, &result, NULL);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/proc/self/cmdline", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/dev/null", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/dev/zero", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/notexist", 1024, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/dev/zero", 102400, &result, &size);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
  err = FileUtil::readFile("/dev/zero", 102400, &result, NULL);
  printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
}

