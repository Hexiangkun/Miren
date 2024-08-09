#include "base/FileUtil.h"
#include "base/Types.h"
#include "base/ErrorInfo.h"
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace Miren
{
namespace base
{
namespace FileUtil
{
    ReadSmallFile::ReadSmallFile(StringArg filename) : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
                            err_(0)
    {
        buf_[0] = '\0';
        if(fd_ < 0) {
            err_ = errno;
        }
    }

    ReadSmallFile::~ReadSmallFile()
    {
        if(fd_ >= 0) {
            ::close(fd_);
        }
    }

    template<typename String>
    int ReadSmallFile::readToString(int maxSize, String* content, int64_t* fileSize, int64_t* modifyTime, int64_t* createTime)
    {
        static_assert(sizeof(off_t) == 8,  "_FILE_OFFSET_BITS = 64");
        assert(content != nullptr);

        int err = err_;
        if(fd_ >= 0) {
            content->clear();

            if(fileSize) {
                struct stat statbuf;
                if(::fstat(fd_, &statbuf) == 0) {
                    if(S_ISREG(statbuf.st_mode)) {
                        *fileSize = statbuf.st_size;
                        content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
                    }
                    else if(S_ISDIR(statbuf.st_mode)) {
                        err = EISDIR;
                    }

                    if(modifyTime) {
                        *modifyTime = statbuf.st_mtime;
                    }
                    if(createTime) {
                        *createTime = statbuf.st_ctime;
                    }
                }
                else {
                    err = errno;
                }
            }

            while(content->size() < implicit_cast<size_t>(maxSize)) {
                size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
                ssize_t n = ::read(fd_, buf_, toRead);
                if(n > 0) {
                    content->append(buf_, n);
                }
                else {
                    if(n < 0) {
                        err = errno;
                    }
                    break;
                }
            }
        }
        return err;
    }

    int ReadSmallFile::readToBuffer(int *size) {
        int err = err_;
        if(fd_ >= 0) {
            ssize_t n = ::pread(fd_, buf_, sizeof(buf_) - 1, 0);//pread和read区别，pread读取完文件offset不会更改;而read会引发offset随读到的字节数移动
            if(n >= 0) {
                if(size) {
                    *size =static_cast<int>(n);
                }
                buf_[n] = '\0';
            } else {
                err = errno;
            }
        }
        return err;
    }

    template int ReadSmallFile::readToString(int maxSize, std::string *content, int64_t *fileSize, int64_t *modifyTime,
                                             int64_t *createTime);

    template int readFile(StringArg filename, int maxSize, std::string* content,
                          int64_t* fileSize, int64_t* modifyTime, int64_t* createTime);



    //----------------------------AppendFile----------------------------------
    //不是线程安全的
    AppendFile::AppendFile(Miren::base::StringArg filename)
            :fp_(::fopen(filename.c_str(), "ae")),
            writtenBytes_(0)
    {
        assert(fp_);
        ::setbuffer(fp_, buffer_, sizeof buffer_);  //设置文件指针fp_的缓冲区设定64K，也就是文件的stream大小
    }

    AppendFile::~AppendFile()
    {
        ::fclose(fp_);
    }
    //不是线程安全的，需要外部加锁
    void AppendFile::append(const char *logline, const size_t len) {
        size_t n = write(logline, len); //返回的n是已经写入文件的字节数
        size_t remain = len - n;        //相减大于0表示未写完
        while (remain > 0) {
            size_t x = write(logline + n, remain);
            if(x == 0) {
                int err = ferror(fp_);
                if(err) {
                    fprintf(stderr, "AppendFile::append() failed %s\n", ErrorInfo::strerror_tl(err));
                }
                break;
            }
            n += x;
            remain = len - n;
        }
        writtenBytes_ += len;   //已经写入的个数
    }

    void AppendFile::flush() {
        ::fflush(fp_);
    }

    size_t AppendFile::write(const char *logline, size_t len) {
        return ::fwrite_unlocked(logline, 1, len, fp_);//不加锁的方式写入，效率高，not thread safe
    }
}
}
}