#pragma once

#include "base/Copyable.h"
#include "base/StringUtil.h"

#include <algorithm>
#include <vector>
#include <string.h>

namespace Miren::net
{
    // buffer底层采用vector实现，readIndex代表开始读的位置，writeIndex代表写的位置
    // 可读的字节数：writeIndex - readIndex
    // 可写的空间为：size - writeIndex

    class Buffer : public base::Copyable
    {
    public:
        static const size_t kCheapPrepend = 8;      // buffer前面预留的字节数
        static const size_t kInitialSize = 1024;    // 初始化大小
        explicit Buffer(size_t initialSize = kInitialSize);
        void swap(Buffer& rhs);

        size_t readableBytes() const { return writerIndex_ - readerIndex_; }
        size_t writableBytes() const { return buffer_.size() - writerIndex_; }

        // 预留空间的大小，readIndex前面的空间都可以作为预留空间
        size_t prependableBytes() const { return readerIndex_; }

        // 可读数据的地址
        const char* peek() const { return begin() + readerIndex_;}
        // 查找'\r\n'
        const char* findCRLF() const;
        const char* findCRLF(const char* start) const;
        // 查找'\n'
        const char* findEOL() const;
        const char* findEOL(const char* start) const;

        // 提取len字节的数据
        void retrieve(size_t len);
        // 提取end之前的所有数据
        void retrieveUntil(const char* end);
        void retrieveInt64() { retrieve(sizeof(int64_t)); }
        void retrieveInt32() { retrieve(sizeof(int32_t)); }
        void retrieveInt16() { retrieve(sizeof(int16_t)); }
        void retrieveInt8() { retrieve(sizeof(int8_t)); }
        void retrieveAll();

        std::string retrieveAllAsString();
        // 提取len字节的数据，并返回string
        std::string retrieveAsString(size_t len);
        base::StringPiece toStringPiece() const;

        void append(const base::StringPiece& str);
        void append(const char* data, size_t len);
        void append(const void* data, size_t len);
  ///
  /// Append int32_t from network endian
  ///
        void appendInt64(int64_t x);
        void appendInt32(int32_t x);
        void appendInt16(int16_t x);
        void appendInt8(int8_t x);

  ///
  /// Read int32_t from network endian
  ///
        int64_t readInt64();
        int32_t readInt32();
        int16_t readInt16();
        int8_t readInt8();

        int64_t peekInt64() const;
        int32_t peekInt32() const;
        int16_t peekInt16() const;
        int8_t peekInt8() const;

        void prependInt64(int64_t x);
        void prependInt32(int32_t x);
        void prependInt16(int16_t x);
        void prependInt8(int8_t x);
        void prepend(const void* data, size_t len);

        void shrink(size_t reserve);

        void ensureWritableBytes(size_t len);

        char* beginWrite() { return begin() + writerIndex_; }
        const char* beginWrite() const { return begin() + writerIndex_; }

        void hasWritten(size_t len);
        void unwritten(size_t len);

        size_t internalCapacity() const;
        ssize_t readFd(int fd, int* savedErrno);
    private:
        char* begin() { return &*buffer_.begin(); }
        const char* begin() const { return&*buffer_.begin(); }

        void makeSpace(size_t len);
    private:
        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;

        static const char kCRLF[];
    };
}