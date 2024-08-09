//
// Created by 37496 on 2024/6/11.
//

#ifndef SERVER_MUTEX_H
#define SERVER_MUTEX_H

#include "base/thread/CurrentThread.h"
#include "base/Noncopyable.h"
#include <assert.h>
#include <pthread.h>

#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY \
  THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
  THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)


#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail (int errnum,
                                  const char *file,
                                  unsigned int line,
                                  const char *function)
    noexcept __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

#else  // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

#endif // CHECK_PTHREAD_RETURN_VALUE


namespace Miren::base
{
    //互斥器(不可拷贝)
    class CAPABILITY("mutex") MutexLock : NonCopyable
    {
    public:
        MutexLock() : holder_(0) {
            MCHECK(pthread_mutex_init(&mutex_, nullptr));
        }

        ~MutexLock() {
            assert(holder_ == 0);   //断言锁没有被任何线程使用
            MCHECK(pthread_mutex_destroy(&mutex_));
        }

        bool isLockedByThisThread() const { //用来检查当前线程是否给这个MutexLock对象加锁
            return holder_ == CurrentThread::tid();
        }

        void assertLocked() const ASSERT_CAPABILITY(this) {
            assert(isLockedByThisThread());
        }

        void lock() ACQUIRE() {
            MCHECK(pthread_mutex_lock(&mutex_));
            assignHolder(); //记录加锁的线程tid
        }

        void unlock() RELEASE() {
            unassignHolder();   //记住清零holder
            MCHECK(pthread_mutex_unlock(&mutex_));
        }

        pthread_mutex_t* getPthreadMutex() {
            return &mutex_;
        }
    private:
        void assignHolder() {    //上锁时记录线程tid给holder_赋值(在上锁后调用)
            holder_ = CurrentThread::tid();
        }

        void unassignHolder() {  //解锁时给holder_置零(在解锁前调用)
            holder_ = 0;
        }
    private:
        friend class Condition; //友元Condition,为的是其能操作MutexLock类

        class UnassignGuard : NonCopyable
        {
        public:
            explicit UnassignGuard(MutexLock& owner) : owner_(owner) {
                owner_.unassignHolder();    //构造函数给holder_置零
            }
            ~UnassignGuard() {
                owner_.assignHolder();      //析构函数给holder_赋线程id
            }
        private:
            MutexLock& owner_;
        };
    private:
        pthread_mutex_t mutex_;
        pid_t holder_;          //用来表示给互斥量上锁线程的tid
    };
    
    //在使用mutex时，有时会忘记给mutex解锁，为了防止这种情况发生，常常使用RAII手法
    class SCOPED_CAPABILITY MutexLockGuard : public NonCopyable
    {
    public:
      explicit MutexLockGuard(MutexLock& mutex) ACQUIRE(mutex): mutex_(mutex)
      {
        mutex_.lock();
      }
      ~MutexLockGuard() RELEASE()
      {
        mutex_.unlock();
      }
    private:
      MutexLock& mutex_;
    };
}

#define MutexLockGuard(x) error "Missing guard object name"

#endif //SERVER_MUTEX_H
