#pragma once

#include "base/Noncopyable.h"
#include "net/Callbacks.h"

#include <google/protobuf/message.h>
#include <map>
#include <type_traits>

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
//依赖注入
class Callback : Miren::base::NonCopyable
{
public:
  virtual ~Callback() = default;
  virtual void onMessage(const Miren::net::TcpConnectionPtr&, const MessagePtr& , Miren::base::Timestamp) const  = 0;
};


template<typename T>
class CallbackT : public Callback
{
static_assert(std::is_base_of<google::protobuf::Message, T>::value, "T must be derived from google::protobuf::Message");
public:
  typedef std::function<void(const Miren::net::TcpConnectionPtr&, const std::shared_ptr<T>&, Miren::base::Timestamp)> ProtobufMessageCallback;
  CallbackT(const ProtobufMessageCallback& callback) : callback_(callback)
  {

  }
  void onMessage(const Miren::net::TcpConnectionPtr& conn, const MessagePtr& message, Miren::base::Timestamp receiveTime) const override
  {
    std::shared_ptr<T> concrete = Miren::down_pointer_cast<T>(message);
    assert(concrete != nullptr);
    callback_(conn, concrete, receiveTime);
  }

private:
  ProtobufMessageCallback callback_;
};

class ProtobufDispatcher
{
  public:
    typedef std::function<void(const Miren::net::TcpConnectionPtr&, const MessagePtr&, Miren::base::Timestamp)> ProtobufMessageCallback;

    explicit ProtobufDispatcher(const ProtobufMessageCallback& cb) : defaultCallback_(cb)
    {

    }

    void onProtobufMessage(const Miren::net::TcpConnectionPtr& conn, const MessagePtr& message, Miren::base::Timestamp receiveTime)
    {
      CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
      if(it != callbacks_.end()) {
        it->second->onMessage(conn, message, receiveTime);
      }
      else {
        defaultCallback_(conn, message, receiveTime);
      }
    }

    template<typename T>
    void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageCallback& callback) 
    {
      std::shared_ptr<CallbackT<T>> pd(new CallbackT<T>(callback));
      callbacks_[T::descriptor()] = pd;
    }

  private:
      ProtobufMessageCallback defaultCallback_;

      typedef std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback>> CallbackMap;
      CallbackMap callbacks_;
};