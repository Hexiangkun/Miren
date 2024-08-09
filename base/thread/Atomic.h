#pragma once

#include <stdint.h>
#include "base/Noncopyable.h"

namespace Miren
{
namespace base
{
namespace detail
{
    //原子操作,是线程安全的，比锁的开销小，里面的CAS没有使用汇编而是使用__sync_val_compare_and_swap这个GCC内置函数
    template<typename T>
    class AtomicIntegerT :  NonCopyable
    {
        public:
            // 初始化
            AtomicIntegerT() : value_(0) {}

            // 获取值
            T get() {
                //比较value的值是否为0，如果是0，返回0；如果value不为0，不设置为0，则直接返回value
                return __sync_val_compare_and_swap(&value_, 0, 0);
            }
            // 先获取值，再加x
            T getAndAdd(T x) {
                return __sync_fetch_and_add(&value_, x);
            }

            // 先加x，再获取值
            T addAndGet(T x) {
                return __sync_add_and_fetch(&value_, x);
            }

            // 返回加1后的值
            T incrementAndGet() {
                return addAndGet(1);
            }

            // 返回减1后的值
            T decrementAndGet() {
                return addAndGet(-1);
            }

            // 加x
            void add(T x) {
                addAndGet(x);
            }

            // 加1
            void increment() {
                incrementAndGet();
            }

            // 减1
            void decrement() {
                decrementAndGet();
            }

            // 获取原来的值，并设置新值
            T getAndSet(T newValue) {
                return __sync_lock_test_and_set(&value_, newValue);
            }
        private:
            volatile T value_;
    };
}

typedef base::detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef base::detail::AtomicIntegerT<int64_t> AtomicInt64;
}


}