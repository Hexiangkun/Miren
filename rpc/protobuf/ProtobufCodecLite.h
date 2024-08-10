#pragma once

#include "base/Noncopyable.h"
#include "base/StringUtil.h"
#include "base/Timestamp.h"
#include "net/Callbacks.h"
#include "base/log/Logging.h"
#include <memory>
#include <type_traits>

namespace google
{
namespace protobuf
{
  class Message;
}
}

namespace Miren
{
namespace net
{
namespace rpc
{

    typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

    // wire format
    //
    // Field     Length  Content
    //
    // size      4-byte  M+N+4
    // tag       M-byte  could be "RPC0", etc.
    // payload   N-byte
    // checksum  4-byte  adler32 of tag+payload
    
    class ProtobufCodecLite : public base::NonCopyable
    {
    public:
    const static int kHeaderLen = sizeof(int32_t);      //存储content长度， 用4个字节
    const static int kCheckSumLen = sizeof(int32_t);    //校验和，4个字节
    const static int kMaxMessageLen = 64*1024*1024;     // same as codec_stream.h kDefaultTotalBytesLimit

    enum ErrorCode
    {
        kNoError = 0,
        kInvalidLength,
        kCheckSumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
    };

    // return false to stop parsing protobuf message
    typedef std::function<bool (const TcpConnectionPtr&,
                                base::StringPiece,
                                base::Timestamp)> RawMessageCallback;

    typedef std::function<void (const TcpConnectionPtr&,
                                const MessagePtr&,
                                base::Timestamp)> ProtobufMessageCallback;

    typedef std::function<void (const TcpConnectionPtr&,
                                Buffer*,
                                base::Timestamp,
                                ErrorCode)> ErrorCallback;

    ProtobufCodecLite(const ::google::protobuf::Message* prototype,
                        base::StringPiece tagArg,
                        const ProtobufMessageCallback& messageCb,
                        const RawMessageCallback& rawCb = RawMessageCallback(),
                        const ErrorCallback& errorCb = defaultErrorCallback)
        : prototype_(prototype),
        tag_(tagArg),
        messageCallback_(messageCb),
        rawCb_(rawCb),
        errorCallback_(errorCb),
        kMinMessageLen(static_cast<int>(tagArg.size() + kCheckSumLen))
    {
        LOG_INFO << "kMinMessageLen : " << kMinMessageLen;
    }

    virtual ~ProtobufCodecLite() = default;

    const std::string& tag() const { return tag_; }

    void send(const TcpConnectionPtr& conn,
                const ::google::protobuf::Message& message);

    void onMessage(const TcpConnectionPtr& conn,
                    Buffer* buf,
                    base::Timestamp receiveTime);

    virtual bool parseFromBuffer(base::StringPiece buf, google::protobuf::Message* message);
    virtual int serializeToBuffer(const google::protobuf::Message& message, Buffer* buf);

    static const std::string& errorCodeToString(ErrorCode errorCode);

    // public for unit tests
    ErrorCode parse(const char* buf, int len, ::google::protobuf::Message* message);
    void fillEmptyBuffer(Miren::net::Buffer* buf, const google::protobuf::Message& message);

    static int32_t checksum(const void* buf, int len);
    static bool validateChecksum(const char* buf, int len);
    static int32_t asInt32(const char* buf);
    static void defaultErrorCallback(const TcpConnectionPtr&,
                                    Buffer*,
                                    base::Timestamp,
                                    ErrorCode);

    private:
    const ::google::protobuf::Message* prototype_;
    const std::string tag_;
    ProtobufMessageCallback messageCallback_;
    RawMessageCallback rawCb_;
    ErrorCallback errorCallback_;
    const int kMinMessageLen;
    };

    template<typename MSG, const char* TAG, typename CODEC = ProtobufCodecLite>
    class ProtobufCodecLiteT
    {
        static_assert(std::is_base_of<ProtobufCodecLite, CODEC>::value, "CODEC should be derived from ProtobufCodecLite");
    public:
        typedef std::shared_ptr<MSG> ConcreteMessagePtr;
        typedef std::function<void (const TcpConnectionPtr&, const ConcreteMessagePtr&, base::Timestamp)> ProtobufMessageCallback;
        typedef ProtobufCodecLite::RawMessageCallback RawMessageCallback;
        typedef ProtobufCodecLite::ErrorCallback ErrorCallback;

        explicit ProtobufCodecLiteT(const ProtobufMessageCallback& messageCb,
                                    const RawMessageCallback& rawCb = RawMessageCallback(),
                                    const ErrorCallback& errorCb = ProtobufCodecLite::defaultErrorCallback)
                :messageCallback_(messageCb),
                codec_(&MSG::default_instance(),
                        TAG,
                        std::bind(&ProtobufCodecLiteT::onRpcMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                        rawCb,
                        errorCb)
        {

        }

        const std::string& tag() const { return codec_.tag(); }

        void send(const TcpConnectionPtr& conn, const MSG& message)
        {
            codec_.send(conn, message);
        }

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, base::Timestamp receiveTime)
        {
            codec_.onMessage(conn, buf, receiveTime);
        }

        void onRpcMessage(const TcpConnectionPtr& conn, const MessagePtr& message, base::Timestamp receiveTime)
        {
            messageCallback_(conn, ::Miren::down_pointer_cast<MSG>(message), receiveTime);
        }

        void fillEmptyBuffer(net::Buffer* buf, const MSG& message)
        {
            codec_.fillEmptyBuffer(buf, message);
        }
    private:
        ProtobufMessageCallback messageCallback_;
        CODEC codec_;
    };
                
} // namespace rpc
} // namespace net

} // namespace Miren
