#include "rpc/RpcCodec.h"
#include "rpc/rpc.pb.h"
#include "rpc/protobuf/ProtobufCodecLite.h"
#include "net/Buffer.h"

#include <stdio.h>

using namespace Miren;
using namespace Miren::net;

void rpcMessageCallback(const TcpConnectionPtr&,
                        const rpc::RpcMessagePtr&,
                        base::Timestamp)
{
}

rpc::MessagePtr g_msgptr;
void messageCallback(const TcpConnectionPtr&,
                     const rpc::MessagePtr& msg,
                     base::Timestamp)
{
  g_msgptr = msg;
}

void print(const Buffer& buf)
{
  printf("encoded to %zd bytes\n", buf.readableBytes());
  for (size_t i = 0; i < buf.readableBytes(); ++i)
  {
    unsigned char ch = static_cast<unsigned char>(buf.peek()[i]);

    printf("%2zd:  0x%02x  %c\n", i, ch, isgraph(ch) ? ch : ' ');
  }
}

char rpctag[] = "RPC0";

int main()
{
  rpc::RpcMessage message;
  message.set_type(rpc::REQUEST);
  message.set_id(2);
  char wire[] = "\0\0\0\x13" "RPC0" "\x08\x01\x11\x02\0\0\0\0\0\0\0" "\x0f\xef\x01\x32";
  std::string expected(wire, sizeof(wire)-1);
  std::string s1, s2;
  Buffer buf1, buf2;
  {
  rpc::RpcCodec codec(rpcMessageCallback);
  codec.fillEmptyBuffer(&buf1, message);
  print(buf1);
  s1 = buf1.toStringPiece();
  }

  {
  rpc::ProtobufCodecLite codec(&rpc::RpcMessage::default_instance(), "RPC0", messageCallback);
  codec.fillEmptyBuffer(&buf2, message);
  print(buf2);
  s2 = buf2.toStringPiece();
  codec.onMessage(TcpConnectionPtr(), &buf1, base::Timestamp::now());
  assert(g_msgptr);
  assert(g_msgptr->DebugString() == message.DebugString());
  g_msgptr.reset();
  }
  assert(s1 == s2);
  assert(s1 == expected);
  assert(s2 == expected);

  {
  Buffer buf;
  rpc::ProtobufCodecLite codec(&rpc::RpcMessage::default_instance(), "XYZ", messageCallback);
  codec.fillEmptyBuffer(&buf, message);
  print(buf);
  s2 = buf.toStringPiece();
  codec.onMessage(TcpConnectionPtr(), &buf, base::Timestamp::now());
  assert(g_msgptr);
  assert(g_msgptr->DebugString() == message.DebugString());
  }

  google::protobuf::ShutdownProtobufLibrary();
}
