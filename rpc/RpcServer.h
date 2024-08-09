#pragma once

#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include <map>
namespace google
{
    namespace protobuf
    {
        class Service;
    }
}

namespace Miren
{
    namespace net
    {
        namespace rpc
        {
        class RpcServer
        {
        public:
            RpcServer(EventLoop* loop, const InetAddress& listenAddr);

            void setThreadNum(int numThreads) {
                server_.setThreadNum(numThreads);
            }

            void registerService(::google::protobuf::Service*);
            void start();

        private:
            void onConnection(const TcpConnectionPtr& conn);
            void onMessage(const TcpConnectionPtr& conn, Buffer* buf, base::Timestamp receiveTime);
        private:
            TcpServer server_;
            std::map<std::string, ::google::protobuf::Service*> services_;
        };

        } // namespace rpc
    } // namespace net
    
} // namespace Miren

