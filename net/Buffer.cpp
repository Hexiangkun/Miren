#include "net/Buffer.h"
#include "net/sockets/Endian.h"
#include "net/sockets/SocketsOps.h"
#include "base/Types.h"
#include "base/log/Logging.h"
#include <assert.h>
#include <sys/uio.h>
#include <errno.h>

namespace Miren
{
    namespace net
    {
        const char Buffer::kCRLF[] = "\r\n";
        const size_t Buffer::kInitialSize;
        const size_t Buffer::kCheapPrepend;


        Buffer::Buffer(size_t initialSize) 
                    : buffer_(kCheapPrepend + initialSize),
                    readerIndex_(kCheapPrepend),
                    writerIndex_(kCheapPrepend)
        {

        }

        void Buffer::swap(Buffer& rhs)
        {
            buffer_.swap(rhs.buffer_);
            std::swap(readerIndex_, rhs.readerIndex_);
            std::swap(writerIndex_, rhs.writerIndex_);
        }

        const char* Buffer::findCRLF() const
        {
            const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
            return crlf == beginWrite() ? nullptr : crlf;
        }

        const char* Buffer::findCRLF(const char* start) const
        {
            assert(peek() <= start);
            assert(start <= beginWrite());
            const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
            return crlf == beginWrite() ? nullptr : crlf;
        }
        const char* Buffer::findEOL() const
        {

            const void* eol = memchr(peek(), '\n', readableBytes());
            return static_cast<const char*>(eol);
        }

        const char* Buffer::findEOL(const char* start) const
        {
            assert(peek() <= start);
            assert(start <= beginWrite());
            const void* eol = memchr(start, '\n', beginWrite() - start);
            return static_cast<const char*>(eol);
        }

        void Buffer::retrieve(size_t len)
        {
            assert(len <= readableBytes());
            if(len < readableBytes()) {
                readerIndex_ += len;
            }
            else {
                retrieveAll();
            }
        }

        void Buffer::retrieveUntil(const char* end)
        {
            assert(peek() <= end);
            assert(end <= beginWrite());
            retrieve(end - peek());
        }

        void Buffer::retrieveAll()
        {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }

        std::string Buffer::retrieveAllAsString()
        {
            return retrieveAsString(readableBytes());
        }

        std::string Buffer::retrieveAsString(size_t len)
        {
            assert(len <= readableBytes());
            std::string result(peek(), len);
            retrieve(len);
            return result;
        }

        base::StringPiece Buffer::toStringPiece() const
        {
            return base::StringPiece(peek(), static_cast<int>(readableBytes()));
        }

        void Buffer::append(const base::StringPiece& str)
        {
            append(str.data(), str.size());
        }
    
        void Buffer::append(const char* data, size_t len)
        {
            ensureWritableBytes(len);
            std::copy(data, data+len, beginWrite());
            hasWritten(len);
        }

        void Buffer::append(const void* data, size_t len)
        {
            append(static_cast<const char*>(data), len);
        }

        void Buffer::appendInt64(int64_t x)
        {
            int64_t be64 = sockets::hostToNetwork64(x);
            append(&be64, sizeof(be64));
        }
        void Buffer::appendInt32(int32_t x)
        {
            int32_t be32 = sockets::hostToNetwork32(x);
            append(&be32, sizeof(be32));
        }

        void Buffer::appendInt16(int16_t x)
        {
            int16_t be16 = sockets::hostToNetwork16(x);
            append(&be16, sizeof(be16));
        }

        void Buffer::appendInt8(int8_t x)
        {
            append(&x, sizeof(x));
        }

        int64_t Buffer::readInt64()
        {
            int64_t result = peekInt64();
            retrieveInt64();
            return result;
        }
        int32_t Buffer::readInt32()
        {
            int32_t result = peekInt32();
            retrieveInt32();
            return result;
        }

        int16_t Buffer::readInt16()
        {
            int16_t result = peekInt16();
            retrieveInt16();
            return result;
        }

        int8_t Buffer::readInt8()
        {
            int8_t result = peekInt8();
            retrieveInt8();
            return result;
        }

        int64_t Buffer::peekInt64() const
        {
            assert(readableBytes() >= sizeof(int64_t));
            int64_t be64 = 0;
            ::memcpy(&be64, peek(), sizeof(be64));
            return sockets::networkToHost64(be64);
        }
        
        int32_t Buffer::peekInt32() const
        {
            assert(readableBytes() >= sizeof(int32_t));
            int32_t be32 = 0;
            ::memcpy(&be32, peek(), sizeof(be32));
            return sockets::networkToHost32(be32);
        }

        int16_t Buffer::peekInt16() const
        {
            assert(readableBytes() >= sizeof(int16_t));
            int16_t be16 = 0;
            ::memcpy(&be16, peek(), sizeof(be16));
            return sockets::networkToHost16(be16);
        }

        int8_t Buffer::peekInt8() const
        {
            assert(readableBytes() >= sizeof(int8_t));
            int8_t x = *peek();
            return x;
        }

        void Buffer::prependInt64(int64_t x)
        {
            int64_t be64 = sockets::hostToNetwork64(x);
            prepend(&be64, sizeof be64);
        }
        void Buffer::prependInt32(int32_t x)
        {
            int32_t be32 = sockets::hostToNetwork32(x);
            prepend(&be32, sizeof be32);
        }
        void Buffer::prependInt16(int16_t x)
        {
            int16_t be16 = sockets::hostToNetwork16(x);
            prepend(&be16, sizeof be16);
        }

        void Buffer::prependInt8(int8_t x)
        {
            prepend(&x, sizeof x);
        }
        void Buffer::prepend(const void* data, size_t len)
        {
            assert(len <= prependableBytes());
            readerIndex_ -= len;
            const char* d = static_cast<const char*>(data);
            std::copy(d, d+len, begin()+readerIndex_);
        }

        void Buffer::shrink(size_t reserve)
        {
            Buffer other;
            other.ensureWritableBytes(readableBytes() + reserve);
            other.append(toStringPiece());
            swap(other);
        }

        void Buffer::ensureWritableBytes(size_t len)
        {
            if(writableBytes() < len) {
                makeSpace(len);
            }
            assert(writableBytes() >= len);
        }

        void Buffer::hasWritten(size_t len)
        {
            assert(len <= writableBytes());
            writerIndex_ += len;
        }

        void Buffer::unwritten(size_t len)
        {
            assert(len <= readableBytes());
            writerIndex_ -= len;
        }

        size_t Buffer::internalCapacity() const
        {
            return buffer_.capacity();
        }

        ssize_t Buffer::readFd(int fd, int* savedErrno)
        {
            ssize_t all = 0;
            char extrabuf[65536];
            struct iovec vec[2];
        read_again:
            const size_t writable = writableBytes();
            // LOG_INFO << writable;
            vec[0].iov_base = begin()+writerIndex_;
            vec[0].iov_len = writableBytes();
            vec[1].iov_base = extrabuf;
            vec[1].iov_len = sizeof extrabuf;

            const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
            const ssize_t n = sockets::readv(fd, vec, iovcnt);
            all += n;
            if(n < 0) {
                *savedErrno = errno;
            }
            else if(base::implicit_cast<size_t>(n) <= writable) {
                writerIndex_ += n;
            }
            else {
                writerIndex_ = buffer_.size();
                append(extrabuf, n-writable);
            }
            if(n == static_cast<ssize_t>(writable) + static_cast<ssize_t>(sizeof extrabuf)) {
                goto read_again;
            }

            return all;
        }

        void Buffer::makeSpace(size_t len)
        {
            if(writableBytes() + prependableBytes() < len + kCheapPrepend) {
                buffer_.resize(writerIndex_ + len);
            }
            else {
                assert(kCheapPrepend < readerIndex_);
                size_t readable = readableBytes();
                std::copy(begin()+readerIndex_, begin()+writerIndex_,
                        begin() + kCheapPrepend);
                readerIndex_ = kCheapPrepend;
                writerIndex_ = kCheapPrepend + readable;
                assert(readableBytes() == readable);
            }
        }

    } // namespace net
    
} // namespace Miren
