#pragma once

#include "base/Noncopyable.h"
#include "base/Types.h"
#include "base/StringUtil.h"

namespace Miren::log
{
namespace detail
{
    const int kSmallBuffer = 4000;      //4k
    const int kLargeBuffer = 4000*1000; //4M

    //一个固定大小SIZE的Buffer类
    template<int SIZE>
    class FixedBuffer : public base::NonCopyable
    {
    public:
        FixedBuffer(): cur_(data_) {
            setCookie(cookieStart); //设置cookie，这个函数目前还没加入功能，所以可以不用管
        }

        ~FixedBuffer() {
            setCookie(cookieEnd);
        }
        //如果可用数据足够，就拷贝buf过去，同时移动当前指针
        void append(const char* buf, size_t len) {
            if(base::implicit_cast<size_t>(avail()) > len) {
                memcpy(cur_, buf, len);
                cur_ += len;
            }
        }
        //返回首地址
        const char* data() const { return data_; }
        //返回缓冲区已有数据长度
        int length() const { return static_cast<int>(cur_ - data_); }
        //返回当前数据末端地址
        char* current() { return cur_; }
        //返回剩余可用地址
        int avail() const { return static_cast<int>(end() - cur_); }
        //cur后移
        void add(size_t len) { cur_ += len; }
        //重置，不清数据，只需要让cur指回首地址即可
        void reset() { cur_ = data_; }
        //清零
        void bzero() { base::MemoryZero(data_, sizeof data_); }
        // for used by GDB
        const char* debugString();
        void setCookie(void (*cookie)()) { cookie_ = cookie; }

        std::string toString() const { return std::string(data_, length()); }
        base::StringPiece toStringPiece() const { return base::StringPiece(data_, length()); }

    private:
        //返回尾指针
        const char* end() const { return data_ + sizeof(data_); }   
        static void cookieStart();
        static void cookieEnd();

    private:
        char data_[SIZE];   //缓冲区数据，大小为size
        char* cur_;         //当前指针,永远指向已有数据的最右端
        void (*cookie_)();
    };
}

//重载了各种<<,负责把各个类型的数据转换成字符串，
//再添加到FixedBuffer中
//该类主要负责将要记录的日志内容放到这个Buffer里面
class LogStream : public base::NonCopyable
{
public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

    LogStream& operator<<(bool v);
    
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float);
    LogStream& operator<<(double);

    LogStream& operator<<(char);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);

    LogStream& operator<<(const std::string&);
    LogStream& operator<<(const base::StringPiece&);
    LogStream& operator<<(const Buffer&);

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck();

    template<typename T>
    void formatInteger(T);
private:
    Buffer buffer_;

    static const int kMaxNumericSize = 48;
};


class Fmt
{
public:
    template<typename T>
    Fmt(const char* fmt, T val) ;

    const char* data() const { return buf_; }
    int length() const { return length_; }
private:
    char buf_[32];
    int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt) {
    s.append(fmt.data(), fmt.length());
    return s;
}
}