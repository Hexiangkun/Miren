#include "net/sockets/SocketsOps.h"
#include "net/sockets/Endian.h"
#include "base/Types.h"
#include "base/log/Logging.h"
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>

namespace Miren::net::sockets
{
    typedef struct sockaddr SA;

#if VALGRIND || defined (NO_ACCEPT4)
void setNonBlockAndCloseOnExec(int sockfd)
{
  // non-block
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  // FIXME check

  (void)ret;
}
#endif

    int createNoblockingOrDie(sa_family_t family)
    {
        #if VALGRIND
            int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
            if(sockfd < 0) {
                LOG_SYSFATAL << "sockets::createNoblockingOrDie";
            }
        #else 
            int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
            if(sockfd < 0) {
                LOG_SYSFATAL << "sockets::createNoblockingOrDie";
            }
        #endif
        return sockfd;
    }

    int createUdpNonblockingOrDie(sa_family_t family)
    {
    #if VALGRIND
        int sockfd = ::socket(family, SOCK_DGRAM, IPPROTO_UDP);

        if (sockfd < 0)
        {
            LOG_SYSFATAL << "sockets::createUdpNonblockingOrDie";
        }

        setNonBlockAndCloseOnExec(sockfd);
    #else
        int sockfd = ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
        if (sockfd < 0)
        {
            LOG_SYSFATAL << "sockets::createUdpNonblockingOrDie";
        }
    #endif
        return sockfd;
    }


    int connect(int sockfd, const struct sockaddr* addr)
    {
        return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    }

    void bindOrDie(int sockfd, const struct sockaddr* addr)
    {
        int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
        if(ret < 0) {
            LOG_SYSFATAL << "sockets::bindOrDie";
        }
    }

    void listenOrDie(int sockfd)
    {
        int ret = ::listen(sockfd, SOMAXCONN);
        if(ret < 0) {
            LOG_SYSFATAL << "sockets::listenOrDie";
        }
    }

    int accept(int sockfd, struct sockaddr_in6* addr)
    {
        socklen_t addr_len = static_cast<socklen_t>(sizeof(*addr));
        #if VALGRIND || defined (NO_ACCEPT4)
            int connfd = ::accept(sockfd, sockaddr_cast(addr), &addr_len);
            setNonBlockAndCloseOnExec(connfd);
        #else
            int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addr_len, SOCK_CLOEXEC | SOCK_NONBLOCK);
        #endif
        if(connfd < 0) {
            int savedErrno = errno;
            LOG_SYSERR << "Socket::accept";
            switch(savedErrno)
            {
                case EAGAIN:
                case ECONNABORTED:
                case EINTR:
                case EPROTO:
                case EPERM:
                case EMFILE:
                    errno = savedErrno;
                    break;
                case EBADF:
                case EFAULT:
                case EINVAL:
                case ENFILE:
                case ENOBUFS:
                case ENOMEM:
                case ENOTSOCK:
                case EOPNOTSUPP:
                    LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                    break;
                default:
                    LOG_FATAL << "unknown error of ::accept " << savedErrno;
                    break;
            }
        }
        return connfd;
    }

    void close(int sockfd)
    {
        
        int ret = ::close(sockfd);
        if(ret < 0) {
            LOG_SYSERR << "sockets::close";
        }
        
    }
    //只关闭写端
    void shutdownWrite(int sockfd)
    {
        if(::shutdown(sockfd, SHUT_WR) < 0) {
            LOG_SYSERR << "sockets::shutdownWrite";
        }
    }

    ssize_t read(int sockfd, void* buf, size_t count)
    {
        return ::read(sockfd, buf, count);
    }
    ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt)
    {
        return ::readv(sockfd, iov, iovcnt);
    }

    ssize_t write(int sockfd, const void* buf, size_t count)
    {
        return ::write(sockfd, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return ::writev(fd, iov, iovcnt);
    }

    ssize_t recvfrom(int sockfd, struct sockaddr_in6* addr, void *packetMem, size_t packetSize)
    {
        socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));

        ssize_t readByteCount = ::recvfrom(sockfd,
            packetMem,
            packetSize,
            0,
            sockaddr_cast(addr),
            &addrlen);

        if (readByteCount < 0)
        {
            int savedErrno = errno;
            LOG_SYSERR << "Socket::recvfrom";
            switch (savedErrno)
            {
                case EAGAIN:
                    // expected errors
                    errno = savedErrno;
                    break;
                default:
                    LOG_FATAL << "unknown error of ::recvfrom " << savedErrno;
                    break;
            }
        }
	    return readByteCount;
    }


    const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr)
    {
        return static_cast<const struct sockaddr*>(base::implicit_cast<const void*>(addr));
    }
    const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr)
    {
        return static_cast<const struct sockaddr*>(base::implicit_cast<const void*>(addr));
    }
    struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr)
    {
        return static_cast<struct sockaddr*>(base::implicit_cast<void*>(addr));
    }
    const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr)
    {
        return static_cast<const struct sockaddr_in*>(base::implicit_cast<const void*>(addr));
    }
    const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr)
    {
        return static_cast<const struct sockaddr_in6*>(base::implicit_cast<const void*>(addr));
    }

    struct sockaddr_in6 getLocalAddr(int sockfd)
    {
        struct sockaddr_in6 localaddr;
        base::MemoryZero(&localaddr, sizeof(localaddr));

        socklen_t addr_len = static_cast<socklen_t>(sizeof localaddr);
        if(::getsockname(sockfd, sockaddr_cast(&localaddr), &addr_len) < 0) {
            LOG_SYSERR << "sockets::getLocalAddr";
        }
        return localaddr;
    }
    struct sockaddr_in6 getPeerAddr(int sockfd)
    {
        struct sockaddr_in6 peeraddr;
        base::MemoryZero(&peeraddr, sizeof(peeraddr));

        socklen_t addr_len = static_cast<socklen_t>(sizeof peeraddr);
        if(::getpeername(sockfd, sockaddr_cast(&peeraddr), &addr_len) < 0) {
            LOG_SYSERR << "sockets::getPeerAddr";
        }
        return peeraddr;
    }

    //判断是否发生了自连接,即源端IP/PORT=目的端IP/PORT
    bool isSelfConnect(int sockfd)
    {
        struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
        struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);

        if(localaddr.sin6_family == AF_INET) {
            const struct sockaddr_in* localaddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
            const struct sockaddr_in* peeraddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
            return localaddr4->sin_port == peeraddr4->sin_port && localaddr4->sin_addr.s_addr == peeraddr4->sin_addr.s_addr;
        }
        else if(localaddr.sin6_family == AF_INET6) {
            return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof(localaddr.sin6_addr)) == 0;
        }
        else {
            return false;
        }
    }

    //将sockaddr地址转换成ip+port的字符串保存到buf
    void toIpPort(char* buf, size_t size, const struct sockaddr* addr)
    {
        toIp(buf, size, addr);
        size_t end = ::strlen(buf);
        const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
        uint16_t port = sockets::networkToHost16(addr4->sin_port);
        assert(size > end);
        snprintf(buf+end, size-end, ":%u", port);
    }
    //从sockaddr获取点分十进制IP地址
    void toIp(char* buf, size_t size, const struct sockaddr* addr)
    {
        if(addr->sa_family == AF_INET) {
            assert(size >= INET_ADDRSTRLEN);
            const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
            ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
        }
        else if(addr->sa_family == AF_INET6) {
            assert(size >= INET6_ADDRSTRLEN);
            const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
        }
    }
    //将点分十进制的ip地址转化为用于网络传输的数值格式
    void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
    {
        addr->sin_family = AF_INET;
        addr->sin_port = sockets::hostToNetwork16(port);
        if(::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
            LOG_SYSERR << "sockets::fromIpPort";
        }
    }
    void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr)
    {
        addr->sin6_family = AF_INET6;
        addr->sin6_port = sockets::hostToNetwork16(port);
        if(::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
            LOG_SYSERR << "sockets::formIpPort";
        }
    }
    //返回socket错误码
    int getSocketError(int sockfd)
    {
        int optval;
        socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
        if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
            return errno;
        }
        else {
            return optval;
        }
    }

}