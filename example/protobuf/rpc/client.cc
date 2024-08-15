#include "example/protobuf/rpc/sudoku.pb.h"

#include "base/log/Logging.h"
#include "net/EventLoop.h"

#include "net/sockets/InetAddress.h"
#include "net/TcpClient.h"
#include "net/TcpConnection.h"
#include "rpc/RpcChannel.h"

#include <stdio.h>
#include <unistd.h>

using namespace Miren;
using namespace Miren::net;
using namespace Miren::net::rpc;

class RpcClient : base::NonCopyable
{
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      client_(loop, serverAddr, "RpcClient"),
      channel_(new RpcChannel),
      stub_(get_pointer(channel_))
  {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, std::placeholders::_1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, get_pointer(channel_), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      //channel_.reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      sudoku::SudokuRequest request;
      request.set_checkerboard("001010");
      sudoku::SudokuResponse* response = new sudoku::SudokuResponse;

      stub_.Solve(NULL, &request, response, NewCallback(this, &RpcClient::solved, response));
    }
    else
    {
      loop_->quit();
    }
  }

  void solved(sudoku::SudokuResponse* resp)
  {
    LOG_INFO << "solved:\n" << resp->DebugString();
    client_.disconnect();
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  sudoku::SudokuService::Stub stub_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    InetAddress serverAddr(argv[1], 9981);

    RpcClient rpcClient(&loop, serverAddr);
    rpcClient.connect();
    loop.loop();
  }
  else
  {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}

