#pragma once

#include "base/Noncopyable.h"
#include "base/thread/Atomic.h"
#include "net/Callbacks.h"
#include "net/sockets/InetAddress.h"
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

        class TcpServer : base::NonCopyable
        {
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
            enum Option
            {
                kNoReusePort,
                kReusePort,
            };

            TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option = kNoReusePort);
            ~TcpServer();

            const std::string& ipPort() const { return ipPort_; }
            const std::string& name() const { return name_; }
            EventLoop* getLoop() const { return loop_; }

            void setThreadNum(int numThreads);
            void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
            std::shared_ptr<EventLoopThreadPool> threadPool() { return threadPool_; }

            void start();

            void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb;}
            void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
            void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
            
        private:
            void newConnection(int sockfd, const InetAddress& peerAddr);
            void removeConnection(const TcpConnectionPtr& conn);
            void removeConnectionInLoop(const TcpConnectionPtr& conn);
        private:
            typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;
            EventLoop* loop_;
            const std::string ipPort_;
            const std::string name_;

            std::unique_ptr<Acceptor> acceptor_;
            std::shared_ptr<EventLoopThreadPool> threadPool_;
            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            ThreadInitCallback threadInitCallback_;
            base::AtomicInt32 started_;
            int nextConnId_;
            ConnectionMap connections_;
        };
    } // namespace net
    
} // namespace Miren
