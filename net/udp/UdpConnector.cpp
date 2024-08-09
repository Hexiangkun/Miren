

#include "net/udp/UdpConnector.h"

#include "base/log/Logging.h"
#include "base/ErrorInfo.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/sockets/SocketsOps.h"

#include <errno.h>

namespace Miren
{
namespace net
{

const int UdpConnector::kMaxRetryDelayMs;

UdpConnector::UdpConnector(EventLoop* loop, const InetAddress& serverAddr, const InetAddress& localAddr)
	:loop_(loop),
	serverAddr_(serverAddr),
	localAddr_(localAddr),
	connect_(false),
	state_(kDisconnected),
	retryDelayMs_(kInitRetryDelayMs)
{
	LOG_DEBUG << "ctor[" << this << "]";
}

UdpConnector::~UdpConnector()
{
	LOG_DEBUG << "dtor[" << this << "]";
}

void UdpConnector::start()
{
	connect_ = true;
	loop_->runInLoop(std::bind(&UdpConnector::startInLoop, this)); // FIXME: unsafe
}

void UdpConnector::startInLoop()
{
	loop_->assertInLoopThread();
	assert(state_ == kDisconnected);
	if (connect_)
	{
		connect();
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

void UdpConnector::stop()
{
	connect_ = false;
	loop_->queueInLoop(std::bind(&UdpConnector::stopInLoop, this)); // FIXME: unsafe
																 // FIXME: cancel timer
}

void UdpConnector::stopInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnecting)
	{
		setState(kDisconnected);
		if(channel_ != nullptr) {
			int sockfd = channel_->fd();
			retry(sockfd);
		}
	}
}

void UdpConnector::restart()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs_ = kInitRetryDelayMs;
	connect_ = true;
	startInLoop();
}

void UdpConnector::connect()
{
	int sockfd = sockets::createUdpNonblockingOrDie(serverAddr_.family());
	Socket* connectSocket(new Socket(sockfd));
	connectSocket->setReuseAddr(true);
	connectSocket->setReusePort(true);
	if(localAddr_.toPort() != 0) {
		connectSocket->bindAddress(localAddr_);
	}

	int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
	int savedErrno = (ret == 0) ? 0 : errno;
	switch (savedErrno)
	{
		case 0:
		case EINPROGRESS:
		case EINTR:
		case EISCONN:
			connected(connectSocket);
			break;

		case EAGAIN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		case ECONNREFUSED:
		case ENETUNREACH:
			retry(sockfd);
			break;

		case EACCES:
		case EPERM:
		case EAFNOSUPPORT:
		case EALREADY:
		case EBADF:
		case EFAULT:
		case ENOTSOCK:
			LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
			sockets::close( sockfd );
			break;

		default:
			LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
			sockets::close( sockfd );
			// connectErrorCallback_();
			break;
	}
}


/////////// new : for UDP
void UdpConnector::connected(Socket* connectedSocket)
{
	setState(kConnecting);
	setState(kConnected);


	if (connect_)
	{
		newUdpConnectionCallback_(connectedSocket);
	}
	else
	{
		// sockets::close(connectedSocket->fd());
	}
}

void UdpConnector::connecting(int sockfd)
{
	setState(kConnecting);

	assert(!channel_);
	channel_.reset(new Channel(loop_, sockfd));
	channel_->setWriteCallback(
		std::bind(&UdpConnector::handleWrite, this)); // FIXME: unsafe
	channel_->setErrorCallback(
		std::bind(&UdpConnector::handleError, this)); // FIXME: unsafe

													 // channel_->tie(shared_from_this()); is not working,
													 // as channel_ is not managed by shared_ptr
	channel_->enableWriting();
}

void UdpConnector::handleWrite()
{
	LOG_TRACE << "Connector::handleWrite " << state_;

	if (state_ == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		if (err)
		{
			LOG_WARN << "Connector::handleWrite - SO_ERROR = "
				<< err << " " << base::ErrorInfo::strerror_tl(err);
			retry(sockfd);
		}
		else if (sockets::isSelfConnect(sockfd))
		{
			LOG_WARN << "Connector::handleWrite - Self connect";
			retry(sockfd);
		}
		else
		{
			setState(kConnected);
			if (connect_)
			{
				newConnectionCallback_( sockfd );
			}
			else
			{
				sockets::close( sockfd );
			}
		}
	}
	else
	{
		// what happened?
		assert(state_ == kDisconnected);
	}
}

void UdpConnector::handleError()
{
	LOG_ERROR << "Connector::handleError state=" << state_;
	if (state_ == kConnecting)
	{
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		LOG_TRACE << "SO_ERROR = " << err << " " << base::ErrorInfo::strerror_tl(err);
		retry(sockfd);
	}
}

void UdpConnector::retry(int sockfd)
{
	//sockets::close( sockfd );
	setState(kDisconnected);
	if (connect_)
	{
		LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
			<< " in " << retryDelayMs_ << " milliseconds. ";
		loop_->runAfter(retryDelayMs_ / 1000.0,
			std::bind(&UdpConnector::startInLoop, shared_from_this()));
		retryDelayMs_ = (retryDelayMs_ * 2) < kMaxRetryDelayMs ?
			(retryDelayMs_ * 2) : kMaxRetryDelayMs;
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

int UdpConnector::removeAndResetChannel()
{
	channel_->disableAll();
	channel_->remove();
	int sockfd = channel_->fd();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop_->queueInLoop(std::bind(&UdpConnector::resetChannel, this)); // FIXME: unsafe
	return sockfd;
}

void UdpConnector::resetChannel()
{
	channel_.reset();
}

  
} // namespace net
} // namespace Miren
