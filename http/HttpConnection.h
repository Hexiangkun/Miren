#pragma once

#include "base/Noncopyable.h"
#include "base/StringUtil.h"
#include "base/Types.h"
#include "net/Buffer.h"
#include "net/sockets/InetAddress.h"
#include "http/ByteData.h"
#include "http/Callbacks.h"
#include <memory>
#include <any>
#include <queue>
#include <map>

struct tcp_info;

namespace Miren
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Socket;
    } // namespace net
    
    namespace http
    {
        class HttpConnection;
        typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;
        class HttpConnection : base::NonCopyable, public std::enable_shared_from_this<HttpConnection>
        {
        public:
            HttpConnection(net::EventLoop* loop, const std::string& name, int sockfd,
                            const net::InetAddress& localAddr, const net::InetAddress& peerAddr);
            ~HttpConnection();

            net::EventLoop* getLoop() const { return loop_; }
            const std::string& name() const { return name_; }
            const net::InetAddress& localAddr() const { return localAddr_; }
            const net::InetAddress& peerAddr() const { return peerAddr_; }
            bool connected() const { return state_ == kConnected; }
            bool disconnected() const { return state_ == kDisconnected; }
            
            bool getTcpInfo(struct tcp_info*) const;
            std::string getTcpInfoString() const;


            void send(const void* data, size_t size);
            void send(ByteData* data);

            void shutdown();
            void forceClose();
            void forceCloseWithDelay(double seconds);
            void setTcpNoDelay(bool on);

            void startRead();
            void stopRead();
            bool isReading() const { return reading_; }

            void setContext(const std::any& context) { context_ = context; }
            const std::any& getContext() const { return context_; }
            std::any& getContext() { return context_; }
            std::any* getMutableContext() { return &context_; }

            void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb;}
            void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
            void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
            void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) { highWaterMarkCallback_ = cb; highWarkMark_ = highWaterMark; }
            void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

            net::Buffer* inputBuffer() { return &inputBuffer_; }
            net::Buffer* outputBuffer() { return &outputBuffer_; }

            void connectEstablished();
            void connectDestroyed();

        private:
            enum CONNSTATE
            {
                kDisconnected,
                kConnecting,
                kConnected,
                kDisconnecting,
            };

        private:
            void handleRead(base::Timestamp receiveTime);
            void handleWrite();

            void handleClose();
            void handleError();

            void sendInLoop(const void* message, size_t len);
            void sendInLoop(ByteData* data);
            void shutdownInLoop();
            void forceCloseInLoop();

            void setState(CONNSTATE s) { state_ = s; }
            const char* stateToString() const;

            void startReadInLoop();
            void stopReadInLoop();

        private:
            net::EventLoop* loop_;
            const std::string name_;    //连接的名字
            CONNSTATE state_;           //tcp连接的状态
            bool reading_;              //是否在读

            std::unique_ptr<net::Socket> socket_;        //fd对应的socket
            std::unique_ptr<net::Channel> channel_;      //fd对应的channel
            const net::InetAddress localAddr_;           //tcp连接中本地的ip，port
            const net::InetAddress peerAddr_;            //tcp连接中对方的ip，port

            ConnectionCallback connectionCallback_;             //连接建立和关闭时的回调函数
            MessageCallback messageCallback_;                   //收到消息时的回调函数
            WriteCompleteCallback writeCompleteCallback_;       //消息写入对方缓冲区时的回调函数
            HighWaterMarkCallback highWaterMarkCallback_;       //高水位回调函数
            CloseCallback closeCallback_;                       //关闭tcp连接的回调函数

            size_t highWarkMark_;
            net::Buffer inputBuffer_;
            net::Buffer outputBuffer_;

            std::queue<ByteData*> send_datas_;
            std::any context_;
            std::map<std::string, std::any> contexts_;
            
        };
    } // namespace http
    
} // namespace Miren
