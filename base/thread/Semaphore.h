#pragma once

#include <semaphore.h>
#include <stdint.h>
#include <stdexcept>
namespace Miren
{
    namespace base
    {
        class Semaphore
        {
        private:
            sem_t semaphore_;
        public:
            Semaphore(uint32_t count = 0)
            {
                if(sem_init(&semaphore_, 0, count)) {
                    throw std::logic_error("sem_init error");
                }
            }

            ~Semaphore()
            {
                sem_destroy(&semaphore_);
            }

            void wait() {
                if(sem_wait(&semaphore_)) {
                    throw std::logic_error("sem_wait error");
                }
            }

            void notify() {
                if(sem_post(&semaphore_)) {
                    throw std::logic_error("sem_post error");
                }
            }
        };
    } // namespace base
    
} // namespace Miren
