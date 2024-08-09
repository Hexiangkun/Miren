#pragma once 

#include "base/Noncopyable.h"
#include "net/sockets/Socket.h"
#include "net/Channel.h"

#include <functional>
namespace Miren
{
    namespace net
    {
        class EventLoop;
        class InetAddress;

        /*
        Acceptor类用来listen、accept，并使用TcpServer::newConnection的回调函数来处理新到来的连接
        Acceptor拥有者是TcpServer
        */
        class Acceptor : base::NonCopyable
        {
        public:
            typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

            Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
            ~Acceptor();

            // 在TcpServer构造函数中设置
            void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

            bool listening() const { return listening_; }
            void listen();      //启动监听套接字
            
        private:
            void handleRead();  //处理新连接到来

            EventLoop* loop_;
            Socket acceptSocket_;       //监听套接字封装
            Channel acceptChannel_;     //注册套接字对应事件
            NewConnectionCallback newConnectionCallback_;   //连接回调函数  
            bool listening_;            //是否正在监听
            int idleFd_;                //解决了服务器中文件描述符达到上限后如何处理的大问题!
        };
    } // namespace net
    
} // namespace Miren
