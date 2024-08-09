#pragma once

#include "base/Noncopyable.h"
#include "base/thread/Atomic.h"
#include "net/sockets/InetAddress.h"
#include "http/Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <map>


namespace Miren
{
    namespace net
    {
        class Acceptor;
        class EventLoop;
        class EventLoopThreadPool;
    }

    namespace http
    {

        class HttpTcpServer : base::NonCopyable
        {
        public:
            typedef std::function<void(net::EventLoop*)> ThreadInitCallback;
            enum Option
            {
                kNoReusePort,
                kReusePort,
            };

            HttpTcpServer(net::EventLoop* loop, const net::InetAddress& listenAddr, const std::string& name, Option option = kNoReusePort);
            ~HttpTcpServer();

            const std::string& ipPort() const { return ipPort_; }
            const std::string& name() const { return name_; }
            net::EventLoop* getLoop() const { return loop_; }

            void setThreadNum(int numThreads);
            void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
            std::shared_ptr<net::EventLoopThreadPool> threadPool() { return threadPool_; }

            void start();

            void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb;}
            void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
            void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
            
        private:
            void newConnection(int sockfd, const net::InetAddress& peerAddr);
            void removeConnection(const HttpConnectionPtr& conn);
            void removeConnectionInLoop(const HttpConnectionPtr& conn);
        private:
            typedef std::map<std::string, HttpConnectionPtr> ConnectionMap;
            net::EventLoop* loop_;
            const std::string ipPort_;
            const std::string name_;

            std::unique_ptr<net::Acceptor> acceptor_;
            std::shared_ptr<net::EventLoopThreadPool> threadPool_;
            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            ThreadInitCallback threadInitCallback_;
            base::AtomicInt32 started_;
            int nextConnId_;
            ConnectionMap connections_;
        };
  }
} // namespace Miren
