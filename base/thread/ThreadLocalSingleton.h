//
// Created by 37496 on 2024/6/16.
//

#ifndef SERVER_THREADLOCALSINGLETON_H
#define SERVER_THREADLOCALSINGLETON_H

#include "base/Noncopyable.h"
#include <pthread.h>
#include <assert.h>

namespace Miren
{
namespace base
{
    template<typename T>
    class ThreadLocalSingleton : NonCopyable
    {
    public:
        ThreadLocalSingleton() = delete;
        ~ThreadLocalSingleton() = delete;

        static T& instance()
        {
            if(!t_value_) {
                t_value_ = new T();
                deleter_.set(t_value_);
            }
            return *t_value_;
        }

        static T* pointer()
        {
            return t_value_;
        }
    private:
        static void destructor(void* obj)
        {
            assert(obj == t_value_);
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy; (void) dummy;
            delete t_value_;
            t_value_ = 0;
        }

    private:

        class Deleter
        {
        public:
            Deleter()
            {
                pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
            }

            ~Deleter()
            {
                pthread_key_delete(pkey_);
            }

            void set(T* newObj)
            {
                assert(pthread_getspecific(pkey_) == nullptr);
                pthread_setspecific(pkey_, newObj);
            }

        public:
            pthread_key_t pkey_;
        };
    private:
        static __thread T* t_value_;
        static Deleter deleter_;
    };

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;
 
} // namespace base
}

#endif //SERVER_THREADLOCALSINGLETON_H
