//
// Created by 37496 on 2024/6/24.
//

#include "base/log/LogStream.h"
#include <algorithm>
#include <limits>
#include <assert.h>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wtautological-compare"
#else
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

namespace Miren::log
{
    namespace detail
    {
        const char digits[] = "9876543210123456789";
        const char* zero = digits + 9;
        static_assert(sizeof(digits) == 20, "wrong number of digits");

        const char digitsHex[] = "0123456789ABCDEF";
        static_assert(sizeof(digitsHex) == 17, "wrong number of digitsHex");

        //实现将一个整数转换成字符串
        template<typename T>
        size_t convert(char buf[], T value) {
            T i = value;
            char* p = buf;

            do {
                int lsd = static_cast<int>(i % 10);//lsd意思是last digit，最后一位数字
                i /= 10;
                *p++ = zero[lsd];
            } while (i != 0);

            if(value < 0) { //如果小于0，加个负号
                *p++ = '-';
            }
            *p = '\0';  //加上\0
            std::reverse(buf, p);

            return p-buf;
        }
        
        //转为十六进制字符串
        size_t convertHex(char buf[], uintptr_t value) {
            uintptr_t i = value;
            char* p = buf;

            do {
               int lsd = static_cast<int>(i % 16);
               i /= 16;
               *p++ = digitsHex[lsd];
            } while (i != 0);

            *p = '\0';
            std::reverse(buf, p);
            return p - buf;
        }

        template class FixedBuffer<kSmallBuffer>;
        template class FixedBuffer<kLargeBuffer>;

        template<int SIZE>
        const char* FixedBuffer<SIZE>::debugString()
        {
            *cur_ = '\0';
            return data_;
        }

        template<int SIZE>
        void FixedBuffer<SIZE>::cookieStart() {

        }

        template<int SIZE>
        void FixedBuffer<SIZE>::cookieEnd() {

        }
    }

    //把int类型的v转换为字符串类型,添加到buffer_中
    template<typename T>
    void LogStream::formatInteger(T v)
    {
        if(buffer_.avail() >= kMaxNumericSize) {
            size_t len = detail::convert(buffer_.current(), v);
            buffer_.add(len);
        }
    }

    LogStream &LogStream::operator<<(bool v)
    {
        if(v) {
            buffer_.append("true", 4);
        }
        else {
            buffer_.append("false", 5);
        }
        return *this;
    }

    LogStream& LogStream::operator<<(short v)
    {
        *this << static_cast<int>(v);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned short v)
    {
        *this << static_cast<unsigned int>(v);
        return *this;
    }

    LogStream& LogStream::operator<<(int v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned int v)
    {
        formatInteger(v);
        return *this;
    }
    LogStream& LogStream::operator<<(long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream& LogStream::operator<<(long long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned long long v)
    {
        formatInteger(v);
        return *this;
    }

    LogStream& LogStream::operator<<(const void* p)
    {
        uintptr_t v = reinterpret_cast<uintptr_t >(p);
        if(buffer_.avail() >= kMaxNumericSize) {
            char* buf = buffer_.current();
            buf[0] = '0';
            buf[1] = 'x';
            size_t len = detail::convertHex(buf+2, v);
            buffer_.add(len+2);
        }
        return *this;
    }

    LogStream& LogStream::operator<<(float v)
    {
        *this << static_cast<double >(v);
        return *this;
    }

    LogStream& LogStream::operator<<(double v)
    {
        if(buffer_.avail() >= kMaxNumericSize) {
            int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
            buffer_.add(len);
        }
        return *this;
    }


    LogStream& LogStream::operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }

    LogStream& LogStream::operator<<(const char* str)
    {
        if(str) {
            buffer_.append(str, strlen(str));
        }
        else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    LogStream& LogStream::operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    LogStream& LogStream::operator<<(const std::string& v)
    {
        buffer_.append(v.c_str(), v.length());
        return *this;
    }

    LogStream& LogStream::operator<<(const base::StringPiece& v)
    {
        buffer_.append(v.data(), v.size());
        return *this;
    }

    LogStream& LogStream::operator<<(const Buffer& v)
    {
        *this << v.toStringPiece();
        return *this;
    }

    void LogStream::staticCheck() {
        static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                "kMaxNumericSize is large enough");
        static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                      "kMaxNumericSize is large enough");
        static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                      "kMaxNumericSize is large enough");
        static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                      "kMaxNumericSize is large enough");
    }

    template<typename T>
    Fmt::Fmt(const char *fmt, T val) {
        static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

        length_ = snprintf(buf_, sizeof(buf_), fmt, val);
        assert(static_cast<size_t>(length_) < sizeof(buf_));
    }

    template Fmt::Fmt(const char *fmt, char);

    template Fmt::Fmt(const char *fmt, short);
    template Fmt::Fmt(const char *fmt, unsigned short);
    template Fmt::Fmt(const char *fmt, int);
    template Fmt::Fmt(const char *fmt, unsigned int);
    template Fmt::Fmt(const char *fmt, long);
    template Fmt::Fmt(const char *fmt, unsigned long);
    template Fmt::Fmt(const char *fmt, long long);
    template Fmt::Fmt(const char *fmt, unsigned long long);

    template Fmt::Fmt(const char *fmt, float);
    template Fmt::Fmt(const char *fmt, double);
}
