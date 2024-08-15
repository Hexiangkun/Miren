#include "query.pb.h"

#include <functional>

#include <iostream>

using namespace std;

class ProtobufDispatcherLite
{
 public:
  typedef std::function<void (google::protobuf::Message* message)> ProtobufMessageCallback;

  explicit ProtobufDispatcherLite(const ProtobufMessageCallback& defaultCb)
    : defaultCallback_(defaultCb)
  {
  }

  void onMessage(google::protobuf::Message* message) const
  {
    CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
    if (it != callbacks_.end())
    {
      it->second(message);
    }
    else
    {
      defaultCallback_(message);
    }
  }

  void registerMessageCallback(const google::protobuf::Descriptor* desc, const ProtobufMessageCallback& callback)
  {
    callbacks_[desc] = callback;
  }

 private:
  typedef std::map<const google::protobuf::Descriptor*, ProtobufMessageCallback> CallbackMap;

  CallbackMap callbacks_;
  ProtobufMessageCallback defaultCallback_;
};

void onQuery(google::protobuf::Message* message)
{
  cout << "onQuery: " << message->GetTypeName() << endl;
  miren::Query* query = dynamic_cast<miren::Query*>(message);
  assert(query != NULL);
}

void onAnswer(google::protobuf::Message* message)
{
  cout << "onAnswer: " << message->GetTypeName() << endl;
  miren::Answer* answer = dynamic_cast<miren::Answer*>(message);
  assert(answer != NULL);
}

void onUnknownMessageType(google::protobuf::Message* message)
{
  cout << "Discarding " << message->GetTypeName() << endl;
}

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ProtobufDispatcherLite dispatcher(onUnknownMessageType);
  dispatcher.registerMessageCallback(miren::Query::descriptor(), onQuery);
  dispatcher.registerMessageCallback(miren::Answer::descriptor(), onAnswer);

  miren::Query q;
  miren::Answer a;
  miren::Empty e;
  dispatcher.onMessage(&q);
  dispatcher.onMessage(&a);
  dispatcher.onMessage(&e);

  google::protobuf::ShutdownProtobufLibrary();
}
