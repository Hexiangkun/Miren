#include "example/protobuf/codec/codec.h"
#include "base/log/Logging.h"
#include "net/sockets/Endian.h"
#include "rpc/google-inl.h"
#include <google/protobuf/descriptor.h>
#include <zlib.h>


void ProtobufCodec::onMessage(const Miren::net::TcpConnectionPtr& conn, Miren::net::Buffer* buf, Miren::base::Timestamp receiveTime)
{
  while(buf->readableBytes() >= kMinMessageLen + kHeaderLen) {
    const int32_t len = buf->peekInt32();
    if(len > kMaxMessageLen || len < kMinMessageLen) {
      errorCallback_(conn, buf, receiveTime, kInvalidLength);
      break;
    }
    else if(buf->readableBytes() >= Miren::base::implicit_cast<size_t>(len + kHeaderLen)) {
      ErrorCode errorCode = kNoError;
      MessagePtr message = parse(buf->peek() + kHeaderLen, len, &errorCode);

      if(errorCode == kNoError && message) {
        messageCallback_(conn, message, receiveTime);
        buf->retrieve(kHeaderLen + len);
      }
      else {
        errorCallback_(conn, buf, receiveTime, errorCode);
        break;
      }
    }
    else {
      break;
    }
  }
}

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len;
//   int32_t  nameLen;
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }

void ProtobufCodec::fillEmptyBuffer(Miren::net::Buffer* buf, const google::protobuf::Message& message)
{
  assert(buf->readableBytes() == 0);

  const std::string& typeName = message.GetTypeName();
  int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);
  buf->appendInt32(nameLen);
  buf->append(typeName.c_str(), nameLen);

  GOOGLE_DCHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

  size_t byte_size = message.ByteSizeLong();
  buf->ensureWritableBytes(byte_size);
  uint8_t* start = reinterpret_cast<uint8_t*>(buf->beginWrite());
  uint8_t* end = message.SerializeWithCachedSizesToArray(start);
  if(size_t(end - start) != byte_size) {
    ByteSizeConsistencyError(static_cast<int>(byte_size), (int)message.ByteSizeLong(), static_cast<int>(end-start));
  }
  buf->hasWritten(byte_size);

  int32_t checkSum = static_cast<int32_t>(::adler32(1, reinterpret_cast<const Bytef*>(buf->peek()), 
                                            static_cast<int>(buf->readableBytes())));
  buf->appendInt32(checkSum);

  assert(buf->readableBytes() == sizeof(nameLen) + nameLen + byte_size + sizeof(checkSum));

  int32_t len = Miren::net::sockets::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
  buf->prepend(&len, sizeof(len));
}

int32_t ProtobufCodec::asInt32(const char* buf)
{
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return Miren::net::sockets::networkToHost32(be32);
}

MessagePtr ProtobufCodec::parse(const char* buf, int len, ErrorCode* errorCode)
{
  MessagePtr message;
  //check sum
  int32_t expectedCheckSum = asInt32(buf + len - kHeaderLen);
  int32_t checkSum = static_cast<int32_t>(::adler32(1, 
                                        reinterpret_cast<const Bytef*>(buf),
                                        static_cast<int>(len - kHeaderLen)));
  if(checkSum == expectedCheckSum) {
    // get message type name
    int32_t nameLen = asInt32(buf);
    if(nameLen >= 2 && nameLen <= len - 2*kHeaderLen) {
      std::string typeName(buf+kHeaderLen, buf + kHeaderLen + nameLen - 1);

      //create message object
      message.reset(createMessage(typeName));
      if(message) {
        const char* data = buf + kHeaderLen + nameLen;
        int32_t dataLen = len - nameLen - 2*kHeaderLen;

        if(message->ParseFromArray(data, dataLen)) {
          *errorCode = kNoError;
        }
        else {
          *errorCode = kParseError;
        }
      }
      else {
        *errorCode = kUnknownMessageType;
      }
    }
    else {
      *errorCode = kInvalidLength;
    }
  }
  else {
    *errorCode = kCheckSumError;
  }
  return message;
}


google::protobuf::Message* ProtobufCodec::createMessage(const std::string& type_name)
{
  google::protobuf::Message* message = nullptr;
  const google::protobuf::Descriptor* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
  if(descriptor) {
    const google::protobuf::Message* prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if(prototype) {
      message = prototype->New();
    }
  }
  return message;
}

void ProtobufCodec::defaultErrorCallback(const Miren::net::TcpConnectionPtr& conn, Miren::net::Buffer* buf, Miren::base::Timestamp receiveTime, ErrorCode errorCode) 
{
  LOG_ERROR << "ProtobufCodec::defaultErrorCallback - " << errorCodeToString(errorCode);
  if(conn && conn->connected()) {
    conn->shutdown();
  }
}


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

const std::string& ProtobufCodec::errorCodeToString(ErrorCode errorCode)
{
  switch (errorCode)
  {
   case kNoError:
     return kNoErrorStr;
   case kInvalidLength:
     return kInvalidLengthStr;
   case kCheckSumError:
     return kCheckSumErrorStr;
   case kInvalidNameLen:
     return kInvalidNameLenStr;
   case kUnknownMessageType:
     return kUnknownMessageTypeStr;
   case kParseError:
     return kParseErrorStr;
   default:
     return kUnknownErrorStr;
  }
}
