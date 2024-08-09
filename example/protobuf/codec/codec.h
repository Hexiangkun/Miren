#pragma once

#include "net/Buffer.h"
#include "net/TcpConnection.h"
#include "base/Noncopyable.h"

#include <google/protobuf/message.h>

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len;
//   int32_t  nameLen;
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }


typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufCodec : Miren::base::NonCopyable
{
public:
  enum ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError
  };


  typedef std::function<void(const Miren::net::TcpConnectionPtr&, const MessagePtr&, Miren::base::Timestamp)> ProtobufMessageCallback;
  typedef std::function<void(const Miren::net::TcpConnectionPtr&, Miren::net::Buffer*, Miren::base::Timestamp, ErrorCode)> ErrorCallback;

  explicit ProtobufCodec(const ProtobufMessageCallback& messageCb)
        :messageCallback_(messageCb),
        errorCallback_(defaultErrorCallback)
  {

  }

  ProtobufCodec(const ProtobufMessageCallback& messageCb, const ErrorCallback& errorCb)
    :messageCallback_(messageCb), errorCallback_(errorCb) 
  {
    
  }

  void onMessage(const Miren::net::TcpConnectionPtr& conn, Miren::net::Buffer* buf, Miren::base::Timestamp receiveTime);
  void send(const Miren::net::TcpConnectionPtr& conn, const google::protobuf::Message& message)
  {
    Miren::net::Buffer buf;
    fillEmptyBuffer(&buf, message);
    conn->send(&buf);
  }

  static const std::string& errorCodeToString(ErrorCode code);
  static void fillEmptyBuffer(Miren::net::Buffer* buf, const google::protobuf::Message& message);
  static google::protobuf::Message* createMessage(const std::string& type_name);
  static MessagePtr parse(const char* buf, int len, ErrorCode* code);

  private:
    static void defaultErrorCallback(const Miren::net::TcpConnectionPtr&, Miren::net::Buffer*, Miren::base::Timestamp, ErrorCode);
    static int32_t asInt32(const char* buf);
    ProtobufMessageCallback messageCallback_;
    ErrorCallback errorCallback_;

    const static int kHeaderLen = sizeof(int32_t);
    const static int kMinMessageLen = 2 * kHeaderLen + 2; // namelen + typename + checksum
    const static int kMaxMessageLen = 64*1024*1024;
};