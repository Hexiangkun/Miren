#include <stdio.h>
#include "base/Singleton.h"
#include "base/thread/CurrentThread.h"
#include "base/thread/Thread.h"

class Test : Miren::base::NonCopyable
{
 public:
  Test()
  {
    printf("tid=%d, constructing %p\n", Miren::base::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("tid=%d, destructing %p %s\n", Miren::base::CurrentThread::tid(), this, name_.c_str());
  }

  const std::string& name() const { return name_; }
  void setName(const std::string& n) { name_ = n; }

 private:
  std::string name_;
};

class TestNoDestroy : Miren::base::NonCopyable
{
 public:
  // Tag member for Singleton<T>
  void no_destroy();

  TestNoDestroy()
  {
    printf("tid=%d, constructing TestNoDestroy %p\n", Miren::base::CurrentThread::tid(), this);
  }

  ~TestNoDestroy()
  {
    printf("tid=%d, destructing TestNoDestroy %p\n", Miren::base::CurrentThread::tid(), this);
  }
};

void threadFunc()
{
  printf("tid=%d, %p name=%s\n",
         Miren::base::CurrentThread::tid(),
         &Miren::base::Singleton<Test>::instance(),
         Miren::base::Singleton<Test>::instance().name().c_str());
  Miren::base::Singleton<Test>::instance().setName("only one, changed");
}

int main()
{
  Miren::base::Singleton<Test>::instance().setName("only one");
  Miren::base::Thread t1(threadFunc);
  t1.start();
  t1.join();
  printf("tid=%d, %p name=%s\n",
         Miren::base::CurrentThread::tid(),
         &Miren::base::Singleton<Test>::instance(),
         Miren::base::Singleton<Test>::instance().name().c_str());
  Miren::base::Singleton<TestNoDestroy>::instance();
  printf("with valgrind, you should see %zd-byte memory leak.\n", sizeof(TestNoDestroy));
}
