//
// Created by 37496 on 2024/6/12.
//

#ifndef SERVER_STRINGPIECE_H
#define SERVER_STRINGPIECE_H

#include <string_view>
#include <string>
namespace Miren
{
namespace base
{
    class StringArg
    {
    public:
        StringArg(const char* str) : str_(str) {}
        StringArg(const std::string& str) :str_(str.c_str()) {}

        const char* c_str() const { return str_; }
    private:
        const char* str_;
    };

    typedef std::string_view StringPiece;
}
}

#endif //SERVER_STRINGPIECE_H
