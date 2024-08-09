#include "example/protobuf/codec/dispatcher_lite.h"

#include "example/protobuf/codec/query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

void onUnknownMessageType(const Miren::net::TcpConnectionPtr&,
                          const MessagePtr& message,
                          Miren::base::Timestamp)
{
  cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

void onQuery(const Miren::net::TcpConnectionPtr&,
             const MessagePtr& message,
             Miren::base::Timestamp)
{
  cout << "onQuery: " << message->GetTypeName() << endl;
  std::shared_ptr<Miren::Query> query = Miren::down_pointer_cast<Miren::Query>(message);
  assert(query != NULL);
}

void onAnswer(const Miren::net::TcpConnectionPtr&,
              const MessagePtr& message,
              Miren::base::Timestamp)
{
  cout << "onAnswer: " << message->GetTypeName() << endl;
  std::shared_ptr<Miren::Answer> answer = Miren::down_pointer_cast<Miren::Answer>(message);
  assert(answer != NULL);
}

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ProtobufDispatcherLite dispatcher(onUnknownMessageType);
  dispatcher.registerMessageCallback(Miren::Query::descriptor(), onQuery);
  dispatcher.registerMessageCallback(Miren::Answer::descriptor(), onAnswer);

  Miren::net::TcpConnectionPtr conn;
  Miren::base::Timestamp t;

  std::shared_ptr<Miren::Query> query(new Miren::Query);
  std::shared_ptr<Miren::Answer> answer(new Miren::Answer);
  std::shared_ptr<Miren::Empty> empty(new Miren::Empty);
  dispatcher.onProtobufMessage(conn, query, t);
  dispatcher.onProtobufMessage(conn, answer, t);
  dispatcher.onProtobufMessage(conn, empty, t);

  google::protobuf::ShutdownProtobufLibrary();
}

