//
// Created by 37496 on 2024/6/25.
//

#ifndef SERVER_INETADDRESS_H
#define SERVER_INETADDRESS_H

#include "base/Copyable.h"
#include "base/StringUtil.h"
#include <netinet/in.h>

namespace Miren
{
    namespace net
    {
        // 前向声明
        namespace sockets
        {
            const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
        }


        class InetAddress : public base::Copyable
        {
        public:
            explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
            InetAddress(base::StringArg ip, uint16_t port, bool ipv6 = false);

            explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr)
            {

            }
            explicit InetAddress(const struct sockaddr_in6& addr6) : addr6_(addr6)
            {

            }

            sa_family_t family() const { return addr_.sin_family; }
            std::string toIP() const;
            std::string toIpPort() const;
            uint16_t toPort() const;

            const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }
            void setSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }

            uint32_t ipNetEndian() const;
            uint16_t portNetEndian() const;

            static bool resolve(base::StringArg hostname, InetAddress* result);
            bool operator<( const InetAddress& other ) const;
            const struct sockaddr_in6* getSockAddr6() const { return &addr6_; }

        private:
            union
            {
                struct sockaddr_in addr_;
                struct sockaddr_in6 addr6_;
            };
        };
    }
}



#endif //SERVER_INETADDRESS_H
