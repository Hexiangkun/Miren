#include "base/BoundedBlockingQueue.h"
#include "base/thread/CurrentThread.h"
#include "base/thread/Thread.h"

#include <string>
#include <vector>

#include <stdio.h>
#include <unistd.h>

using namespace Miren;

class Test
{
 public:
  Test(int numThreads)
    : queue_(20),
      latch_(numThreads),
      threads_(numThreads)
  {
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(new base::Thread(
            std::bind(&Test::threadFunc, this), std::string(name)));
    }
    for (auto& thr : threads_)
    {
      thr->start();
    }
  }

  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();
    printf("all threads started\n");
    for (int i = 0; i < times; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "hello %d", i);
      queue_.put(buf);
      printf("tid=%d, put data = %s, size = %zd\n", base::CurrentThread::tid(), buf, queue_.size());
    }
  }

  void joinAll()
  {
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put("stop");
    }

    for (auto& thr : threads_)
    {
      thr->join();
    }
  }

 private:

  void threadFunc()
  {
    printf("tid=%d, %s started\n",
           base::CurrentThread::tid(),
           base::CurrentThread::name());

    latch_.countDown();
    bool running = true;
    while (running)
    {
      std::string d(queue_.take());
      printf("tid=%d, get data = %s, size = %zd\n", base::CurrentThread::tid(), d.c_str(), queue_.size());
      running = (d != "stop");
    }

    printf("tid=%d, %s stopped\n",
           base::CurrentThread::tid(),
           base::CurrentThread::name());
  }

  base::BoundedBlockingQueue<std::string> queue_;
  base::CountDownLatch latch_;
  std::vector<std::unique_ptr<base::Thread>> threads_;
};

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), base::CurrentThread::tid());
  Test t(1);
  t.run(100);
  t.joinAll();

  printf("number of created threads %d\n", base::Thread::numCreated());
}
