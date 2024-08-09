//
// Created by 37496 on 2024/6/25.
//

#include "net/sockets/InetAddress.h"
#include "net/sockets/SocketsOps.h"
#include "net/sockets/Endian.h"
#include "base/Types.h"
#include "base/log/Logging.h"
#include <stddef.h>
#include <netdb.h>
#include <netinet/in.h>
#include <assert.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"


namespace Miren
{
    namespace net
    {
        static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");
        static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
        static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
        static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
        static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

        
        InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
        {
            static_assert(offsetof(InetAddress, addr_) == 0 , "addr_ offset 0");
            static_assert(offsetof(InetAddress, addr6_) == 0 , "addr6_ offset 0");
            
            if(ipv6) {
                base::MemoryZero(&addr6_, sizeof(addr6_));
                addr6_.sin6_family = AF_INET6;
                in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
                addr6_.sin6_addr = ip;
                addr6_.sin6_port = sockets::hostToNetwork16(port);
            }
            else {
                base::MemoryZero(&addr_, sizeof(addr_));
                addr_.sin_family = AF_INET;
                in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
                addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
                addr_.sin_port = sockets::hostToNetwork16(port);
            }
        }

        InetAddress::InetAddress(base::StringArg ip, uint16_t port, bool ipv6)
        {
            if(ipv6) {
                base::MemoryZero(&addr6_, sizeof(addr6_));
                sockets::fromIpPort(ip.c_str(), port, &addr6_);
            }
            else {
                base::MemoryZero(&addr_, sizeof(addr_));
                sockets::fromIpPort(ip.c_str(), port, &addr_);
            }
        }

        std::string InetAddress::toIP() const
        {
            char buf[64] = "";
            sockets::toIp(buf, sizeof buf, getSockAddr());
            return buf;
        }

        std::string InetAddress::toIpPort() const
        {
            char buf[64] = "";
            sockets::toIpPort(buf, sizeof buf, getSockAddr());
            return buf;
        }

        uint16_t InetAddress::toPort() const
        {
            return sockets::networkToHost16(portNetEndian());
        }

        uint32_t InetAddress::ipNetEndian() const
        {
            assert(family() == AF_INET);
            return addr_.sin_addr.s_addr;
        }

        uint16_t InetAddress::portNetEndian() const
        {
            return addr_.sin_port;
        }

        static __thread char t_resolveBuffer[64 * 1024];
        bool InetAddress::resolve(base::StringArg hostname, InetAddress* result)
        {
            assert(result != nullptr);
            struct hostent hent;
            struct hostent* he = nullptr;

            int herrno = 0;
            base::MemoryZero(&hent, sizeof(hent));

            int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
            if(ret == 0 && he != nullptr) {
                assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t)); 
                result->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
                return true;
            }
            else {
                if(ret) {
                    LOG_SYSERR << "InetAddress::resolve";
                }
                return false;
            }
        }


bool InetAddress::operator<( const InetAddress& other ) const
{
	const struct sockaddr_in6* thisAddr = this->getSockAddr6();
	const struct sockaddr_in6* otherAddr = other.getSockAddr6();
	if ( thisAddr->sin6_family == AF_INET )
	{
		const struct sockaddr_in* laddr4 = reinterpret_cast< const struct sockaddr_in* >( thisAddr );
		const struct sockaddr_in* raddr4 = reinterpret_cast< const struct sockaddr_in* >( otherAddr );
		if ( laddr4->sin_addr.s_addr < raddr4->sin_addr.s_addr )
		{
			return true;
		}
		else if ( laddr4->sin_port < raddr4->sin_port )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if ( thisAddr->sin6_family == AF_INET6 )
	{
		if ( memcmp( &thisAddr->sin6_addr, &otherAddr->sin6_addr, sizeof thisAddr->sin6_addr ) < 0 )
		{
			return true;
		}
		else if ( thisAddr->sin6_port < otherAddr->sin6_port )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

    }
}
