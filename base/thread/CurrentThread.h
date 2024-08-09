#pragma once

#include "base/Types.h"

namespace Miren
{
namespace base
{
namespace CurrentThread
{
    //__thread修饰的变量是线程局部存储的，否则是多个线程共享
    extern __thread int t_cachedThreadId;               //线程真实pid(tid)的缓存，减少::syscall(SYS_gettid)调用次数
    extern __thread char t_cachedThreadIdString[32];    //tid的字符串表示形式
    extern __thread int t_cachedThreadIdStrLength;      //上述字符串的长度
    extern __thread const char* t_threadName;           //线程名称


    void cacheTid();    //缓存tid到t_cachedTid

    inline int tid()      //返回线程tid
    {   
        //如果未缓存过线程tid，则缓存tid到t_cachedTid
        if(__builtin_expect(t_cachedThreadId == 0, 0)) {
            cacheTid();
        }
        return t_cachedThreadId;
    }

    inline const char* tidString() {
        return t_cachedThreadIdString;
    }

    inline const char* name()
    {
        return t_threadName;
    }

    inline int tidStringLength() {
        return t_cachedThreadIdStrLength;
    }

    inline const char* tidThreadName() {
        return t_threadName;
    }

    //在Thread.h\Thread.cpp里面实现
    bool isMainThread();                //判断是不是主线程
    void sleepUsec(int64_t usec);       //休眠

}
} // namespace base
}