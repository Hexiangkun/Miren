#pragma once

namespace Miren::base
{
    class NonCopyable
    {
        public:
            NonCopyable(const NonCopyable& other) = delete;
            NonCopyable& operator=( const NonCopyable& ) = delete;
        protected:
            NonCopyable() = default;
            ~NonCopyable() = default;
    };
}