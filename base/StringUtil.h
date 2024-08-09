//
// Created by 37496 on 2024/6/12.
//

#ifndef SERVER_STRINGPIECE_H
#define SERVER_STRINGPIECE_H

#include <string_view>
#include <string>
namespace Miren::base
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

    class StringUtil {
    public:
        static std::string Format(const char* fmt, ...);
        static std::string Formatv(const char* fmt, va_list ap);

        static std::string UrlEncode(const std::string& str, bool space_as_plus = true);
        static std::string UrlDecode(const std::string& str, bool space_as_plus = true);

        static std::string Trim(const std::string& str, const std::string& delimit = " \t\r\n");
        static std::string TrimLeft(const std::string& str, const std::string& delimit = " \t\r\n");
        static std::string TrimRight(const std::string& str, const std::string& delimit = " \t\r\n");


        static std::string WStringToString(const std::wstring& ws);
        static std::wstring StringToWString(const std::string& s);

    };


    typedef std::string_view StringPiece;
}


#endif //SERVER_STRINGPIECE_H
