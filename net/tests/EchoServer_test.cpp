#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "net/sockets/InetAddress.h"
#include "base/log/Logging.h"
#include "base/thread/CurrentThread.h"
#include <sys/types.h>
#include <unistd.h>
using namespace Miren;
int numThreads = 0;
class EchoServer
{
public:
    EchoServer(net::EventLoop* loop, const net::InetAddress& listenAddr)
        :loop_(loop), server_(loop, listenAddr, "EchoServer")
    {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_.setThreadNum(numThreads);
    }

    void start()
    {
        server_.start();
    }
private:
    void onConnection(const net::TcpConnectionPtr& conn)
    {
        LOG_TRACE << conn->peerAddr().toIpPort() << " -> "
                << conn->localAddr().toIpPort() << " is " 
                << (conn->connected() ? "UP" : "DOWN");
        LOG_INFO << conn->getTcpInfoString() ;

        conn->send("hello world\n");
    }

    void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buf, base::Timestamp time)
    {
        std::string msg(buf->retrieveAllAsString());
        LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << time.toString();
        if(msg == "exit\n") {
            conn->send("bye\n");
            conn->shutdown();
        }
        if(msg == "quit\n") {
            loop_->quit();
        }
        conn->send(msg);
    }

private:
    net::EventLoop* loop_;
    net::TcpServer server_;
};

int main()
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
    LOG_INFO << "sizeof TcpConnection = " << sizeof(net::TcpConnection);

    net::InetAddress listenAddr(8888, false, false);
    net::EventLoop loop;
    EchoServer server(&loop, listenAddr);
    
    server.start();
    loop.loop();
}