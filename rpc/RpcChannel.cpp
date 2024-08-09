#include "rpc/RpcChannel.h"
#include "log/Logging.h"
#include "rpc/rpc.pb.h"
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <assert.h>
namespace Miren
{
namespace net
{
    namespace rpc
    {
        RpcChannel::RpcChannel()
                    :codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
                    services_(nullptr)
        {
            LOG_INFO << "RpcChannel::ctor - " << this;
        }

        RpcChannel::RpcChannel(const TcpConnectionPtr& conn)
                    :codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
                    services_(nullptr),
                    conn_(conn)
        {
            LOG_INFO << "RpcChannel::ctor - " << this;
        }

        RpcChannel::~RpcChannel()
        {
            LOG_INFO << "RpcChannel::dtor - " << this;
            for(const auto& outstanding : outstandings_) {
                OutstandingCall out = outstanding.second;
                delete out.response;
                delete out.done;
            }
        }

        void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                ::google::protobuf::RpcController* controller,
                const ::google::protobuf::Message* request,
                ::google::protobuf::Message* response,
                ::google::protobuf::Closure* done) 
        {
            RpcMessage message;
            message.set_type(REQUEST);
            int64_t id = id_.incrementAndGet();
            message.set_id(id);
            message.set_service(method->service()->full_name());
            message.set_method(method->name());
            message.set_request(request->SerializeAsString());

            OutstandingCall out = { response, done };
            {
                base::MutexLockGuard lock(mutex_);
                outstandings_[id] = out;
            }
            codec_.send(conn_, message);
        }

        void RpcChannel::onMessage(const TcpConnectionPtr& conn, Miren::net::Buffer* buf, base::Timestamp receiveTime)
        {
            codec_.onMessage(conn, buf, receiveTime);
        }

        void RpcChannel::onRpcMessage(const TcpConnectionPtr& conn, const RpcMessagePtr& messagePtr, base::Timestamp receiveTime)
        {
            assert(conn == conn_);
            RpcMessage& message = *messagePtr;

            if(message.type() == RESPONSE) {
                int64_t id = message.id();
                assert(message.has_response() || message.has_error());

                OutstandingCall out = { nullptr, nullptr};
                {
                    base::MutexLockGuard lock(mutex_);
                    std::map<int64_t, OutstandingCall>::iterator it = outstandings_.find(id);
                    if(it != outstandings_.end()) {
                        out = it->second;
                        outstandings_.erase(it);
                    } 
                }

                if(out.response) {
                    std::unique_ptr<google::protobuf::Message> d(out.response);
                    if(message.has_response()) {
                        out.response->ParseFromString(message.response());
                    }
                    if(out.done) {
                        out.done->Run();
                    }
                }
            }
            else if(message.type() == REQUEST) {
                ErrorCode errorCode = WRONG_PROTO;
                if(services_) {
                    std::map<std::string, google::protobuf::Service*>::const_iterator it = services_->find(message.service());
                    if(it != services_->end()) {
                        google::protobuf::Service* service = it->second;
                        assert(service != nullptr);
                        const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
                        const google::protobuf::MethodDescriptor* method = desc->FindMethodByName(message.method());
                        if(method) {
                            std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
                            if(request->ParseFromString(message.request())) {
                                google::protobuf::Message* response = service->GetResponsePrototype(method).New();

                                int64_t id = message.id();
                                service->CallMethod(method, nullptr, get_pointer(request), response, 
                                                NewCallback(this, &RpcChannel::doneCallback, response, id));
                                
                                errorCode = NO_ERROR;
                            }
                            else {
                                errorCode = INVALID_REQUEST;
                            }
                        }
                        else {
                            errorCode = NO_METHOD;
                        }
                    }
                    else {
                        errorCode = NO_SERVICE;
                    }
                }
                else {
                    errorCode = NO_SERVICE;
                }
                if(errorCode != NO_ERROR) {
                    RpcMessage response;
                    response.set_type(RESPONSE);
                    response.set_id(message.id());
                    response.set_error(errorCode);
                    codec_.send(conn_, response);
                }
            }
            else if(message.type() == ERROR) {
                
            }
    
        }
        void RpcChannel::doneCallback(::google::protobuf::Message* response, int64_t id)
        {
            std::unique_ptr<google::protobuf::Message> d(response);
            RpcMessage message;
            message.set_type(RESPONSE);
            message.set_id(id);
            message.set_response(response->SerializeAsString());
            codec_.send(conn_, message);
        }
        
    }
}
}