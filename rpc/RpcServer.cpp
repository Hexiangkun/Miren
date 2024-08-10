#include "rpc/RpcServer.h"
#include "rpc/RpcChannel.h"
#include "base/log/Logging.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
namespace Miren
{
    namespace net
    {
        namespace rpc
        {
        
        RpcServer::RpcServer(EventLoop* loop, const InetAddress& listenAddr)
            :server_(loop, listenAddr, "RpcServer")
        {
            server_.setConnectionCallback(std::bind(&RpcServer::onConnection, this, std::placeholders::_1));
            // server_.setMessageCallback(std::bind(&RpcServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        void RpcServer::registerService(::google::protobuf::Service* service)
        {
            const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
            services_[desc->full_name()] = service;
        }

        void RpcServer::start()
        {
            server_.start();
        }

        void RpcServer::onConnection(const TcpConnectionPtr& conn)
        {
            LOG_INFO << "RpcServer - " << conn->peerAddr().toIpPort() << " -> "
                    << conn->localAddr().toIpPort() << " is " 
                    << (conn->connected() ? "UP" : "DOWN");
            if(conn->connected()) {
                RpcChannelPtr channel(new RpcChannel(conn));
                channel->setServices(&services_);
                conn->setMessageCallback(std::bind(&RpcChannel::onMessage, get_pointer(channel), 
                                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                conn->setContext("con" ,channel);
            }
            else {
                conn->setContext("con", RpcChannelPtr());
            }
        }

        void RpcServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, base::Timestamp receiveTime)
        {
            RpcChannelPtr& channel = std::any_cast<RpcChannelPtr&>(conn->getContext("con"));
            channel->onMessage(conn, buf, receiveTime);
        }

        } // namespace rpc
    } // namespace net
    
} // namespace Miren
