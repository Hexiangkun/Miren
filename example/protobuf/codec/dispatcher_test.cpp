#include "example/protobuf/codec/dispatcher.h"

#include "example/protobuf/codec/query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

typedef std::shared_ptr<Miren::Query> QueryPtr;
typedef std::shared_ptr<Miren::Answer> AnswerPtr;

void test_down_pointer_cast()
{
  ::std::shared_ptr<google::protobuf::Message> msg(new Miren::Query);
  ::std::shared_ptr<Miren::Query> query(Miren::down_pointer_cast<Miren::Query>(msg));
  assert(msg && query);
  if (!query)
  {
    abort();
  }
}

void onQuery(const Miren::net::TcpConnectionPtr&,
             const QueryPtr& message,
             Miren::base::Timestamp)
{
  cout << "onQuery: " << message->GetTypeName() << endl;
}

void onAnswer(const Miren::net::TcpConnectionPtr&,
              const AnswerPtr& message,
              Miren::base::Timestamp)
{
  cout << "onAnswer: " << message->GetTypeName() << endl;
}

void onUnknownMessageType(const Miren::net::TcpConnectionPtr&,
                          const MessagePtr& message,
                          Miren::base::Timestamp)
{
  cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  test_down_pointer_cast();

  ProtobufDispatcher dispatcher(onUnknownMessageType);
  dispatcher.registerMessageCallback<Miren::Query>(onQuery);
  dispatcher.registerMessageCallback<Miren::Answer>(onAnswer);

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

