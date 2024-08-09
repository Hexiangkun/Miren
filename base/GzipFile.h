#pragma once

#include "base/Noncopyable.h"
#include "base/StringUtil.h"
#include <zlib.h>

namespace Miren
{
namespace base
{


    class GzipFile : NonCopyable
    {
    private:
        explicit GzipFile(gzFile file) : file_(file) {}
    
    public:
        GzipFile(GzipFile&& rhs) noexcept : file_(rhs.file_) {
            rhs.file_ = nullptr;
        }

        ~GzipFile() {
            if(file_) {
                ::gzclose(file_);
            }
        }

        GzipFile& operator=(GzipFile&& rhs) noexcept {
            swap(rhs);
            return *this;
        }

        void swap(GzipFile& rhs) { std::swap(file_, rhs.file_); }
        bool valid() const {return file_!= nullptr; }

        int read(void* buf, int len) { return ::gzread(file_, buf, len); }
        int write(base::StringPiece buf) { return ::gzwrite(file_, buf.data(), static_cast<unsigned>(buf.size())); }
        off_t tell() const { return ::gztell(file_); }


    #if ZLIB_VERNUM >= 0x1240
        bool setBuffer(int size) { return ::gzbuffer(file_, size) == 0; }
        off_t offset() const { return ::gzoffset(file_); }
    #endif

        static GzipFile openForRead(base::StringArg filename) {
            return GzipFile(::gzopen(filename.c_str(), "rbe"));
        }

        static GzipFile openForAppend(base::StringArg filename) {
            return GzipFile(::gzopen(filename.c_str(), "abe"));
        }

        static GzipFile openForWriteExclusive(base::StringArg filename) {
            return GzipFile(::gzopen(filename.c_str(), "wbxe"));
        }

        static GzipFile openForWriteTruncate(base::StringArg filename) {
            return GzipFile(::gzopen(filename.c_str(), "wbe"));
        }
    private:
        gzFile file_;
    };
    
} // namespace base
}
