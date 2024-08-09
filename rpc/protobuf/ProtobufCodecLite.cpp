#include "rpc/protobuf/ProtobufCodecLite.h"
#include "rpc/protobuf/BufferStream.h"
#include "rpc/google-inl.h"

#include "net/sockets/Endian.h"
#include "net/TcpConnection.h"
#include <google/protobuf/message.h>

#include <zlib.h>

namespace
{
    int ProtobufVersionCheck()
    {
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        return 0;
    }

    int __attribute__ ((unused)) dummy = ProtobufVersionCheck();
}

namespace Miren
{
namespace net
{
namespace rpc
{
namespace
{
    const std::string kNoErrorStr = "NoError";
    const std::string kInvalidLengthStr = "InvalidLength";
    const std::string kCheckSumErrorStr = "CheckSumError";
    const std::string kInvalidNameLenStr = "InvalidNameLen";
    const std::string kUnknownMessageTypeStr = "UnknownMessageType";
    const std::string kParseErrorStr = "ParseError";
    const std::string kUnknownErrorStr = "UnknownError";
}

    void ProtobufCodecLite::send(const TcpConnectionPtr& conn, const ::google::protobuf::Message& message)
    {
        Miren::net::Buffer buf;
        fillEmptyBuffer(&buf, message);
        conn->send(&buf);
    }

    // len | tag + message + check
    void ProtobufCodecLite::fillEmptyBuffer(Miren::net::Buffer* buf, const ::google::protobuf::Message& message)
    {
        assert(buf->readableBytes() == 0);
        // FIXME: can we move serialization & checksum to other thread?
        buf->append(tag_);

        int byte_size = serializeToBuffer(message, buf);

        int32_t checkSum = checksum(buf->peek(), static_cast<int>(buf->readableBytes()));
        buf->appendInt32(checkSum);
        assert(buf->readableBytes() == tag_.size() + byte_size + kCheckSumLen); (void) byte_size;
        int32_t len = sockets::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
        buf->prepend(&len, sizeof len);
    }


    void ProtobufCodecLite::onMessage(const TcpConnectionPtr& conn, Buffer* buf, base::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= static_cast<uint32_t>(kMinMessageLen+kHeaderLen))
        {
            const int32_t len = buf->peekInt32();
            LOG_INFO << "len : " << len;
            if (len > kMaxMessageLen || len < kMinMessageLen)
            {
                errorCallback_(conn, buf, receiveTime, kInvalidLength);
                break;
            }
            else if (buf->readableBytes() >= base::implicit_cast<size_t>(kHeaderLen+len))
            {
                if (rawCb_ && !rawCb_(conn, base::StringPiece(buf->peek(), kHeaderLen+len), receiveTime))
                {
                    buf->retrieve(kHeaderLen+len);
                    continue;
                }
                MessagePtr message(prototype_->New());
                // FIXME: can we move deserialization & callback to other thread?
                ErrorCode errorCode = parse(buf->peek()+kHeaderLen, len, message.get());
                if (errorCode == kNoError)
                {
                    LOG_INFO << "knoERRor";
                    // FIXME: try { } catch (...) { }
                    messageCallback_(conn, message, receiveTime);
                    buf->retrieve(kHeaderLen+len);
                }
                else
                {
                    errorCallback_(conn, buf, receiveTime, errorCode);
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }


    ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(const char* buf, int len, ::google::protobuf::Message* message)
    {
        ErrorCode errorCode = kNoError;

        if (validateChecksum(buf, len))
        {
            if (memcmp(buf, tag_.data(), tag_.size()) == 0)
            {
                // parse from buffer
                const char* data = buf + tag_.size();
                int32_t dataLen = len - kCheckSumLen - static_cast<int>(tag_.size());
                if (parseFromBuffer(base::StringPiece(data, dataLen), message))
                {
                    errorCode = kNoError;
                }
                else
                {
                    errorCode = kParseError;
                }
            }
            else
            {
                errorCode = kUnknownMessageType;
            }
        }
        else {
            errorCode = kCheckSumError;
        }
        return errorCode;
    }

    int32_t ProtobufCodecLite::checksum(const void* buf, int len)
    {
        return static_cast<int32_t>(::adler32(1, static_cast<const Bytef*>(buf), len));
    }

    bool ProtobufCodecLite::validateChecksum(const char* buf, int len)
    {
        int32_t expectedCheckSum = asInt32(buf + len - kCheckSumLen);
        int32_t checkSum = checksum(buf, len - kCheckSumLen);
        return checkSum == expectedCheckSum;
    }

    int32_t ProtobufCodecLite::asInt32(const char* buf)
    {
        int32_t be32 = 0;
        ::memcpy(&be32, buf, sizeof(be32));
        return sockets::networkToHost32(be32);
    }

    void ProtobufCodecLite::defaultErrorCallback(const TcpConnectionPtr& conn, Buffer*, base::Timestamp, ErrorCode errorCode)
    {
        LOG_ERROR << "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(errorCode);
        if(conn && conn->connected()) {
            conn->shutdown();
        }
    }

    const std::string& ProtobufCodecLite::errorCodeToString(ErrorCode errorCode)
    {
        switch (errorCode)
        {
        case kNoError:
            return kNoErrorStr;
        case kInvalidLength:
            return kInvalidLengthStr;
        case kCheckSumError:
            return kCheckSumErrorStr;
        case kUnknownMessageType:
            return kUnknownMessageTypeStr;
        case kInvalidNameLen:
            return kInvalidNameLenStr;
        case kParseError:
            return kParseErrorStr;
        default:
            return kUnknownErrorStr;
        }
    }

    bool ProtobufCodecLite::parseFromBuffer(base::StringPiece buf, google::protobuf::Message* message)
    {
        return message->ParseFromArray(buf.data(), static_cast<int>(buf.size()));
    }

    int ProtobufCodecLite::serializeToBuffer(const ::google::protobuf::Message& message, Buffer* buf)
    {
        GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

        #if GOOGLE_PROTOBUF_VERSION > 3009002
            int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
        #else
            int byte_size = message.ByteSize();
        #endif
            buf->ensureWritableBytes(byte_size + kCheckSumLen);

        uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
        uint8_t* end = message.SerializeWithCachedSizesToArray(start);
        if (end - start != byte_size)
        {
            #if GOOGLE_PROTOBUF_VERSION > 3009002
            ByteSizeConsistencyError(byte_size, google::protobuf::internal::ToIntSize(message.ByteSizeLong()), static_cast<int>(end - start));
            #else
            ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
            #endif
        }
        buf->hasWritten(byte_size);
        return byte_size;
    }

} // namespace rpc
} // namespace net

} // namespace Miren
