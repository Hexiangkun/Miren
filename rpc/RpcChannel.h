#pragma once

#include "base/thread/Atomic.h"
#include "base/thread/Mutex.h"
#include "rpc/RpcCodec.h"
#include <google/protobuf/service.h>
#include <map>

namespace google
{
    namespace protobuf
    {
        class Descriptor;
        class ServiceDescriptor;
        class MethodDescriptor;
        class Message;
        class Closure;
        class RpcController;
        class Service;
    } // namespace protobuf  
} // namespace google

namespace Miren
{
namespace net{
    namespace rpc
    {
        class RpcChannel : public ::google::protobuf::RpcChannel
        {
        public:
            RpcChannel();
            explicit RpcChannel(const TcpConnectionPtr& conn);
            ~RpcChannel() override;

            void setConnection(const TcpConnectionPtr& conn) { conn_ = conn; }
            void setServices(const std::map<std::string, ::google::protobuf::Service*>* services) { services_ = services; }

            void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                            ::google::protobuf::RpcController* controller,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done) override;
            
            void onMessage(const TcpConnectionPtr& conn, Miren::net::Buffer* buf, base::Timestamp receiveTime);
        private:
            void onRpcMessage(const TcpConnectionPtr& conn, const RpcMessagePtr& messagePtr, base::Timestamp receiveTime);
            void doneCallback(::google::protobuf::Message* response, int64_t id);
            
        private:
            struct OutstandingCall
            {
                ::google::protobuf::Message* response;
                ::google::protobuf::Closure* done;
            };
            RpcCodec codec_;
            TcpConnectionPtr conn_;
            base::AtomicInt64 id_;
            base::MutexLock mutex_;
            std::map<int64_t, OutstandingCall> outstandings_ GUARDED_BY(mutex_);
            const std::map<std::string, ::google::protobuf::Service*>* services_;
        };

        typedef std::shared_ptr<RpcChannel> RpcChannelPtr;
    }
} // namespace net
    
} // namespace Miren

