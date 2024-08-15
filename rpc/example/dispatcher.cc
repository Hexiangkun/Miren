#include "query.pb.h"

#include <memory>
#include <functional>

#include <iostream>

using namespace std;

class NonCopyable
{
    public:
        NonCopyable(const NonCopyable& other) = delete;
        NonCopyable& operator=( const NonCopyable& ) = delete;
    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;
};

class Callback : public NonCopyable
{
 public:
  virtual ~Callback() {};
  virtual void onMessage(google::protobuf::Message* message) const = 0;
};


template <typename T>
class CallbackT : public Callback
{
 public:
  typedef std::function<void (T* message)> ProtobufMessageCallback;

  CallbackT(const ProtobufMessageCallback& callback)
    : callback_(callback)
  {
  }

  virtual void onMessage(google::protobuf::Message* message) const
  {
    T* t = dynamic_cast<T*>(message);
    assert(t != NULL);
    callback_(t);
  }

 private:
  ProtobufMessageCallback callback_;
};

void discardProtobufMessage(google::protobuf::Message* message)
{
  cout << "Discarding " << message->GetTypeName() << endl;
}

class ProtobufDispatcher
{
 public:

  ProtobufDispatcher()
    : defaultCallback_(discardProtobufMessage)
  {
  }

  void onMessage(google::protobuf::Message* message) const
  {
    CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
    if (it != callbacks_.end())
    {
      it->second->onMessage(message);
    }
    else
    {
      defaultCallback_(message);
    }
  }

  template<typename T>
  void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageCallback& callback)
  {
    std::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(callback));
    callbacks_[T::descriptor()] = pd;
  }

  typedef std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback> > CallbackMap;
  CallbackMap callbacks_;
  std::function<void (google::protobuf::Message* message)> defaultCallback_;
};

//
// test
//

void onQuery(miren::Query* query)
{
  cout << "onQuery: " << query->GetTypeName() << endl;
}

void onAnswer(miren::Answer* answer)
{
  cout << "onAnswer: " << answer->GetTypeName() << endl;
}

int main()
{
  ProtobufDispatcher dispatcher;
  dispatcher.registerMessageCallback<miren::Query>(onQuery);
  dispatcher.registerMessageCallback<miren::Answer>(onAnswer);

  miren::Query q;
  miren::Answer a;
  miren::Empty e;
  dispatcher.onMessage(&q);
  dispatcher.onMessage(&a);
  dispatcher.onMessage(&e);

  google::protobuf::ShutdownProtobufLibrary();
}

