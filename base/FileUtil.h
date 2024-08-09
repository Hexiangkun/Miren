#pragma once

#include "base/Noncopyable.h"
#include "base/StringUtil.h"
#include <sys/types.h>
#include <stdexcept>


namespace Miren
{
namespace base
{
namespace FileUtil
{
    // read small file < 64KB
    //小文件读取的类封装
    class ReadSmallFile : public NonCopyable
    {
    public:
        static const int kBufferSize = 64 * 1024;

        ReadSmallFile(StringArg filename);
        ~ReadSmallFile();
        //该函数用于将小文件的内容转换为字符串
        template<typename String>
        int readToString(int maxSize,           //期望读取的大小
                        String* content,        //要读入的content缓冲区
                        int64_t* fileSize,      //读取出的整个文件大小
                        int64_t* modifyTime,    //读取出的文件修改的时间
                        int64_t* createTime);   //读取出的创建文件的时间

        int readToBuffer(int* size);            //从文件读取数据到buf_

        const char* buffer() const { return buf_; }

    private:
        int fd_;
        int err_;
        char buf_[kBufferSize];
    };

    //一个全局函数，readFile函数，调用ReadSmallFile类中的readToString方法，供外部将小文件中的内容转化为字符串。
    template<typename String>
    int readFile(StringArg filename, int maxSize, String* content, 
            int64_t* fileSize = nullptr, int64_t* modifyTime = nullptr, int64_t* createTime = nullptr)
    {
        ReadSmallFile file(filename);
        return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
    }

    //封装了一个文件指针的操作类,用于把数据写入文件中
    class AppendFile : NonCopyable
    {
    public:
        explicit AppendFile(StringArg filename);

        void append(const char* logline, const size_t len);

        void flush();

        off_t writtenBytes() const { return writtenBytes_; }

        ~AppendFile();

    private:
        size_t write(const char* logline, size_t len);

    private:
        FILE* fp_;
        char buffer_[64*1024];  //文件缓冲区，64K
        off_t writtenBytes_;    //已经写入的字节数

    };

    class File : NonCopyable
    {
    public:
        File(const char * file) : fp_(::fopen(file, "rb")) {

        }
        ~File()
        {
            if(fp_) {
                ::fclose(fp_);
            }
        }

        bool valid() const { return  fp_; }

        std::string readBytes(int n) {
            char buf[n];
            ssize_t nr = ::fread(buf, 1, n, fp_);
            if(nr != n) {
                throw std::logic_error("no enough data");
            }
            return std::string(buf, n);
        }

        int32_t readInt32() {
            int32_t x = 0;
            ssize_t nr = ::fread(&x, 1, sizeof(int32_t), fp_);
            if(nr != sizeof(int32_t)) {
                throw std::logic_error("bad int32_t data");
            }
            return x;
        }

        uint8_t readUInt8() {
            uint8_t x = 0;
            ssize_t nr = ::fread(&x, 1, sizeof(uint8_t), fp_);
            if(nr != sizeof(uint8_t)) {
                throw std::logic_error("bad uint8_t data");
            }
            return x;
        }

    private:
        FILE* fp_;
    };
    
}
} // namespace base
}