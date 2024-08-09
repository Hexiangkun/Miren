#pragma once


#include "base/Noncopyable.h"
#include "net/Callbacks.h"

#include <google/protobuf/message.h>
#include <map>

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufDispatcherLite : Miren::base::NonCopyable
{
  public:
    typedef std::function<void(const Miren::net::TcpConnectionPtr&, const MessagePtr&, Miren::base::Timestamp)> ProtobufMessageCallback;

    explicit ProtobufDispatcherLite(const ProtobufMessageCallback& cb) : defaultCallback_(cb)
    {

    }

    void onProtobufMessage(const Miren::net::TcpConnectionPtr& conn, const MessagePtr& message, Miren::base::Timestamp receiveTime)
    {
      CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
      if(it != callbacks_.end()) {
        it->second(conn, message, receiveTime);
      }
      else {
        defaultCallback_(conn, message, receiveTime);
      }
    }

    void registerMessageCallback(const google::protobuf::Descriptor* desc, const ProtobufMessageCallback& callback) 
    {
      callbacks_[desc] = callback;
    }

  private:
      ProtobufMessageCallback defaultCallback_;

      typedef std::map<const google::protobuf::Descriptor*, ProtobufMessageCallback> CallbackMap;
      CallbackMap callbacks_;
};

