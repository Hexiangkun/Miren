//
// Created by 37496 on 2024/6/25.
//

#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include "base/Noncopyable.h"

struct tcp_info;

namespace Miren
{
    namespace net
    {
        class InetAddress;

        //用RAII方法封装socket file descriptor
        class Socket : base::NonCopyable
        {
        public:
            explicit Socket(int sockfd) : sockfd_(sockfd) {

            }

            ~Socket();

            int fd() const { return sockfd_; }

            bool getTcpInfo(struct tcp_info*) const;
            bool getTcpInfoString(char* buf, int len) const;

            void bindAddress(const InetAddress& localAddr);

            void listen();
            /// 成功时，返回一个非负整数，即
            /// 已接受套接字的描述符，该描述符已
            /// 设置为非阻塞且在执行时关闭。*peeraddr 已分配。
            /// 出错时，返回 -1，*peeraddr 保持不变。
            int accept(InetAddress* peeraddr);

            void shutdownWrite();
            /// Nagle算法可以一定程度上避免网络拥塞
            /// TCP_NODELAY选项可以禁言Nagle算法
            /// 禁用Nagle算法，可以避免连续发包出现延迟，这对于编写低延迟的网络服务很重要
            void setTcpNoDelay(bool on);
            /// Enable/disable SO_REUSEADDR
            void setReuseAddr(bool on);
            /// Enable/disable SO_REUSEPORT
            void setReusePort(bool on);
            /// TCP keepalive是指定期探测连接是否存在，如果应用层有心跳的话，这个选项不是必需要设置的
            void setKeepAlive(bool on);
        private:
            const int sockfd_;  //socket文件描述符
        };
    }
}


#endif //SERVER_SOCKET_H
