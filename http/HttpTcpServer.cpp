#include "http/HttpTcpServer.h"
#include "http/HttpConnection.h"

#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/sockets/SocketsOps.h"

#include "base/log/Logging.h"
#include <stdio.h>

namespace Miren
{
namespace http
{
    HttpTcpServer::HttpTcpServer(net::EventLoop* loop, const net::InetAddress& listenAddr, const std::string& name, Option option)
            :loop_(loop),
            ipPort_(listenAddr.toIpPort()),
            name_(name),
            acceptor_(new net::Acceptor(loop, listenAddr, option == kReusePort)),
            threadPool_(new net::EventLoopThreadPool(loop, name_)),
            connectionCallback_(defaultConnectionCallback),
            messageCallback_(defaultMessageCallback),
            nextConnId_(1)
    {
        acceptor_->setNewConnectionCallback(std::bind(&HttpTcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
    }

    HttpTcpServer::~HttpTcpServer()
    {
        loop_->assertInLoopThread();
        LOG_TRACE << "HttpTcpServer::~HttpTcpServer [" << name_ << "] destructing";

        for(auto& item : connections_) {
            HttpConnectionPtr conn(item.second);
            item.second.reset();
            conn->getLoop()->runInLoop(std::bind(&HttpConnection::connectDestroyed, conn));
        }
    }

    void HttpTcpServer::setThreadNum(int numThreads) 
    {
        assert(0 <= numThreads);
        threadPool_->setThreadNum(numThreads);
    }

    void HttpTcpServer::start()
    {
        if(started_.getAndSet(1) == 0) {
            threadPool_->start(threadInitCallback_);
            assert(!acceptor_->listening());
            loop_->runInLoop(std::bind(&net::Acceptor::listen, get_pointer(acceptor_)));
        }
    }

    void HttpTcpServer::newConnection(int sockfd, const net::InetAddress& peerAddr)
    {
        loop_->assertInLoopThread();
        net::EventLoop* ioLoop = threadPool_->getNextLoop();
        char buf[64];
        snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
        ++nextConnId_;
        std::string connName = name_ + buf;
        LOG_INFO << "HttpTcpServer::newConnection [" << name_
                << "] - new connection [" << connName
                << "] from " << peerAddr.toIpPort();
        net::InetAddress localAddr(net::sockets::getLocalAddr(sockfd));

        HttpConnectionPtr conn = std::make_shared<HttpConnection>(ioLoop, connName, sockfd, localAddr, peerAddr);
        connections_[connName] = conn;
        conn->setConnectionCallback(connectionCallback_);
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        conn->setCloseCallback(std::bind(&HttpTcpServer::removeConnection, this, std::placeholders::_1));
        ioLoop->runInLoop(std::bind(&HttpConnection::connectEstablished, conn));
    }

    void HttpTcpServer::removeConnection(const HttpConnectionPtr& conn)
    {
        loop_->runInLoop(std::bind(&HttpTcpServer::removeConnectionInLoop, this, conn));
    }

    void HttpTcpServer::removeConnectionInLoop(const HttpConnectionPtr& conn)
    {
        loop_->assertInLoopThread();
        LOG_INFO << "HttpTcpServer::removeConnectionInLoop [" << name_
                << "] - connection " << conn->name();
        
        size_t n = connections_.erase(conn->name());
        (void)n;
        assert(n == 1);
        net::EventLoop* ioLoop = conn->getLoop();
        ioLoop->queueInLoop(std::bind(&HttpConnection::connectDestroyed, conn));
    }
} // namespace http

}  // namespace Miren



