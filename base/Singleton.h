#pragma once

#include "base/Noncopyable.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

namespace Miren::base
{
    namespace detail
    {
        template<typename T>
        struct has_no_destroy
        {
            template<typename C> static char test(decltype(&C::no_destroy));
            template<typename C> static int32_t test(...);
            const static bool value = sizeof(test<T>(0)) == 1;
        };
    }

/*
单例模式需要：
  1.私有构造函数  
  2.一个静态方法，返回这个唯一实例的引用
  3.一个指针静态变量
  4.选择一个解决多线程问题的方法
*/
//线程安全的singleton
    template<typename T>
    class Singleton : public NonCopyable
    {
    public:
        static T& instance()
        {
            pthread_once(&ponce_, &Singleton::init);    //使用初值为PTHREAD_ONCE_INIT的变量保证init函数在本进程执行序列中只执行一次
            assert(value_ != nullptr);
            return *value_;
        }

    private:
        Singleton() = default;  //私有构造函数，使得外部不能创建临时对象
        ~Singleton() = default; //私有析构函数，保证只能在堆上new一个新的类对象
        static void init()
        {
            value_ = new T();
            if(!detail::has_no_destroy<T>::value) {
                ::atexit(destroy);      //登记销毁函数，在整个程序结束时会自动调用该函数来销毁对象
            }
        }

        // 如果 T 是一个不完整的类型（例如，只进行了前向声明），
        // 那么 sizeof(T) 的结果将为0，从而导致数组大小为-1。
        // 这样就会导致编译错误，从而提醒开发者需要使用一个完整的类型作为 T。
        static void destroy()
        {
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy; (void) dummy;
            delete value_;
            value_ = nullptr;
        }

    private:
        static pthread_once_t ponce_;   //这个对象保证函数只被执行一次
        static T* value_;               //指针静态变量
    };

    //静态变量均需要初始化
    template<typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template<typename T>
    T* Singleton<T>::value_ = nullptr;
}