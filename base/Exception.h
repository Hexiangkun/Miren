#pragma once

#include <exception>
#include <string>

namespace Miren
{
namespace base
{

    class Exception : public std::exception
    {
    public:
        Exception(std::string msg);
        ~Exception() noexcept override = default;

        const char* what() const noexcept override
        {
            return message_.c_str();
        }

        const char* stackTrace() const noexcept
        {
            return stack_.c_str();
        }

    private:
        std::string message_;   //异常信息的字符串
        std::string stack_;     //保存异常发生时的栈回溯信息
    };

} // namespace base
} // namespace Miren
