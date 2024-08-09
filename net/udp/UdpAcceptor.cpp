#include "net/udp/UdpAcceptor.h"

#include "base/log/Logging.h"
#include "net/EventLoop.h"
#include "net/sockets/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>


namespace Miren
{
namespace net
{


UdpAcceptor::UdpAcceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
	: loop_(loop),
	acceptSocket_(sockets::createUdpNonblockingOrDie(listenAddr.family())),
	acceptChannel_(loop, acceptSocket_.fd()),
	listenning_(false),
	listenPort_(listenAddr.toPort()),
	listenAddr_(listenAddr)
{
	acceptSocket_.setReuseAddr(true);
	acceptSocket_.setReusePort(reuseport);
	acceptSocket_.bindAddress(listenAddr);

	acceptChannel_.setReadCallback(
		std::bind(&UdpAcceptor::handleRead, this));
}

UdpAcceptor::~UdpAcceptor()
{
	acceptChannel_.disableAll();
	acceptChannel_.remove();
}

void UdpAcceptor::listen()
{
	loop_->assertInLoopThread();
	listenning_ = true;

	acceptChannel_.enableReading();
}

void UdpAcceptor::handleRead()
{
	loop_->assertInLoopThread();
	InetAddress peerAddr;

	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	ssize_t readByteCount = sockets::recvfrom(acceptSocket_.fd(), &addr, recvfromBuf_, krecvfromBufSize);

	if (readByteCount >= 0)
	{
		peerAddr.setSockAddrInet6(addr);
		//peerAddr.setSockAddrIn(addr);
		if(newConnectorCallback_) {
			newConnectorCallback_(peerAddr);
		}
		else {
			LOG_SYSERR << "in UdpAcceptor::handleRead newConnectorCallback_";
		}
	}
	else
	{
		LOG_SYSERR << "in UdpAcceptor::handleRead";
	}
}

} // namespace net

} // namespace Miren
