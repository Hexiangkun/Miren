#pragma once

#include "base/Noncopyable.h"
#include "base/StringUtil.h"
#include "base/Types.h"
#include "net/Callbacks.h"
#include "net/Buffer.h"
// #include "net/ByteData.h"
#include "net/sockets/InetAddress.h"
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

        //和新连接相关的所有内容统一封装到该类
        //继承了enable_shared_from_this类，保证返回的对象时shared_ptr类型
        //生命期依靠shared_ptr管理（即用户和库共同控制）
        class TcpConnection : base::NonCopyable, public std::enable_shared_from_this<TcpConnection>
        {
        public:
            TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                            const InetAddress& localAddr, const InetAddress& peerAddr);
            ~TcpConnection();

            EventLoop* getLoop() const { return loop_; }
            const std::string& name() const { return name_; }
            const InetAddress& localAddr() const { return localAddr_; }
            const InetAddress& peerAddr() const { return peerAddr_; }
            bool connected() const { return state_ == kConnected; }
            bool disconnected() const { return state_ == kDisconnected; }
            
            bool getTcpInfo(struct tcp_info*) const;
            std::string getTcpInfoString() const;


            void send(const void* message, int len);
            void send(const base::StringPiece& message);
            void send(Buffer* message);

            //new for http
            // void send(ByteData*);

            void shutdown();
            void forceClose();
            void forceCloseWithDelay(double seconds);
            void setTcpNoDelay(bool on);

            void startRead();
            void stopRead();
            bool isReading() const { return reading_; }

            void setContext(const std::string& name, const std::any& context) { std::any ctx = context; contexts_[name] = ctx; }
            const std::any& getContext(const std::string& name) const { return contexts_.at(name); }
            std::any& getContext(const std::string& name) { return contexts_.at(name); }
            std::any* getMutableContext(const std::string& name) { return &contexts_.at(name); }
            void deleteContext(const std::string& name) { contexts_.erase(name); }


            void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb;}
            void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
            void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
            void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) { highWaterMarkCallback_ = cb; highWarkMark_ = highWaterMark; }
            /// Internal use only.
            /// 这是给TcpServer和TcpClient用的,不是给用户用的
            /// 普通用户用的是ConnectionCallback
            void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

            Buffer* inputBuffer() { return &inputBuffer_; }
            Buffer* outputBuffer() { return &outputBuffer_; }

            // 提供给tcpserver使用
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

            void sendInLoop(const base::StringPiece& message);
            void sendInLoop(const void* message, size_t len);
            void shutdownInLoop();
            void forceCloseInLoop();//用于主动关闭连接

            //new for http
            // void sendInLoop(ByteData* data);

            void setState(CONNSTATE s) { state_ = s; }
            const char* stateToString() const;

            void startReadInLoop();
            void stopReadInLoop();

        private:
            EventLoop* loop_;           //TcpConnection所属的loop
            const std::string name_;    //连接的名字
            CONNSTATE state_;           //tcp连接的状态
            bool reading_;              //是否在读

            std::unique_ptr<Socket> socket_;        //fd对应的socket
            std::unique_ptr<Channel> channel_;      //fd对应的channel
            const InetAddress localAddr_;           //tcp连接中本地的ip，port
            const InetAddress peerAddr_;            //tcp连接中对方的ip，port

            ConnectionCallback connectionCallback_;             //连接建立和关闭时的回调函数
            MessageCallback messageCallback_;                   //收到消息时的回调函数
            WriteCompleteCallback writeCompleteCallback_;       //消息写入对方缓冲区时的回调函数
            HighWaterMarkCallback highWaterMarkCallback_;       //高水位回调函数
            CloseCallback closeCallback_;                       //关闭tcp连接的回调函数

            size_t highWarkMark_;
            Buffer inputBuffer_;
            Buffer outputBuffer_;
            std::vector<Buffer> send_datas_;
            
            std::map<std::string, std::any> contexts_;
            
        };
    } // namespace net
    
} // namespace Miren
