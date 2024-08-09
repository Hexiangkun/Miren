#include "base/GzipFile.h"
#include <iostream>
#include <string.h>
int main()
{
  const char* filename = "/tmp/gzipfile_test.gz";
  ::unlink(filename);
  const char data[] = "123456789012345678901234567890123456789012345678901234567890\n";
  {
  Miren::base::GzipFile writer = Miren::base::GzipFile::openForAppend(filename);
  if (writer.valid())
  {
    std::cout << "tell " << writer.tell();
    std::cout << "wrote " << writer.write(data);
    std::cout << "tell " << writer.tell();
  }
  }

  {
  printf("testing reader\n");
  Miren::base::GzipFile reader = Miren::base::GzipFile::openForRead(filename);
  if (reader.valid())
  {
    char buf[256];
    std::cout << "tell " << reader.tell();
    int nr = reader.read(buf, sizeof buf);
    printf("read %d\n", nr);
    if (nr >= 0)
    {
      buf[nr] = '\0';
      printf("data %s", buf);
    }
    std::cout << "tell " << reader.tell();
    if (strncmp(buf, data, strlen(data)) != 0)
    {
      printf("failed!!!\n");
      abort();
    }
    else
    {
      printf("PASSED\n");
    }
  }
  }

  {
  Miren::base::GzipFile writer = Miren::base::GzipFile::openForWriteExclusive(filename);
  if (writer.valid() || errno != EEXIST)
  {
    printf("FAILED\n");
  }
  }
}
