#pragma once

#include "net/Buffer.h"
#include "base/log/Logging.h"
#include <google/protobuf/io/zero_copy_stream.h>

namespace Miren
{
namespace net
{
namespace rpc
{
            
class BufferOutputStream : public google::protobuf::io::ZeroCopyOutputStream
{
public:
    BufferOutputStream(Buffer* buf) 
                        : buffer_(CHECK_NOTNULL(buf)), 
                        originalSize_(buffer_->readableBytes())
    {

    }

    // 用于获取下一个写入位置的指针和可写入的大小，并在写入后更新缓冲区状态
    virtual bool Next(void** data, int* size)
    {
        buffer_->ensureWritableBytes(4096);
        *data = buffer_->beginWrite();
        *size = static_cast<int>(buffer_->writableBytes());
        buffer_->hasWritten(*size);
        return true;
    }
    // 用于撤销上一次写入操作，即回退已写入的字节数。
    virtual void BackUp(int count)
    {
        buffer_->unwritten(count);
    }
    // 返回自写入开始以来写入的字节数
    virtual int64_t ByteCount() const 
    {
        return buffer_->readableBytes() - originalSize_;
    }

private:
    Buffer* buffer_;
    size_t originalSize_;
};

} // namespace rpc
        
} // namespace net
    
} // namespace Miren
