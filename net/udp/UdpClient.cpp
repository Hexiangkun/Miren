

#include "net/udp/UdpClient.h"

#include "base/log/Logging.h"
#include "net/udp/UdpConnector.h"
#include "net/EventLoop.h"
#include "net/sockets/SocketsOps.h"

#include <stdio.h>  // snprintf
#include "third_party/kcpp/kcpp.h"

namespace Miren
{
namespace net
{
  namespace detail
  {
    void removeUdpConnection( EventLoop* loop, const UdpConnectionPtr& conn )
    {
      loop->queueInLoop( std::bind( &UdpConnection::connectDestroyed, conn ) );
    }

    void removeUdpConnector( const UdpConnectorPtr& connector )
    {
      //connector->
    }
  }



UdpClient::UdpClient( EventLoop* loop,
	const InetAddress& serverAddr,
	const std::string& nameArg )
	: loop_( CHECK_NOTNULL( loop ) ),
	connector_( new UdpConnector( loop, serverAddr, InetAddress(0) ) ),
	name_( nameArg ),
	connectionCallback_( UdpDefaultConnectionCallback ),
	messageCallback_( UdpDefaultMessageCallback ),
	retry_( false ),
	connect_( true ),
	nextConnId_( 1 )
{
	connector_->setNewUdpConnectionCallback(
		std::bind( &UdpClient::newConnection, this,  std::placeholders::_1 ) );
	// FIXME setConnectFailedCallback
	LOG_INFO << "UdpClient::UdpClient[" << name_
		<< "] - connector " << get_pointer( connector_ );
}

UdpClient::~UdpClient()
{
	LOG_INFO << "UdpClient::~UdpClient[" << name_
		<< "] - connector " << get_pointer( connector_ );
	UdpConnectionPtr conn;
	bool unique = false;
	{
		base::MutexLockGuard lock( mutex_ );
		unique = connection_.unique();
		conn = connection_;
	}
	if ( conn )
	{
		assert( loop_ == conn->getLoop() );
		// FIXME: not 100% safe, if we are in different thread
		UdpCloseCallback cb = std::bind( &detail::removeUdpConnection, loop_, std::placeholders::_1 );
		loop_->runInLoop(
			std::bind( &UdpConnection::setCloseCallback, conn, cb ) );
		if ( unique )
		{
			conn->forceClose();
		}
	}
	else
	{
		connector_->stop();
		// FIXME: HACK
		loop_->runAfter( 1, std::bind( &detail::removeUdpConnector, connector_ ) );
	}
}

void UdpClient::connect()
{
	// FIXME: check state
	LOG_INFO << "UdpClient::connect[" << name_ << "] - connecting to "
		<< connector_->serverAddress().toIpPort();
	connect_ = true;
	connector_->start();
}

void UdpClient::disconnect()
{
	connect_ = false;

	{
		base::MutexLockGuard lock( mutex_ );
		if ( connection_ )
		{
			connection_->shutdown();
		}
	}
}

void UdpClient::stop()
{
	connect_ = false;
	connector_->stop();
}

void UdpClient::newConnection(Socket* connectedSocket)
{
	loop_->assertInLoopThread();

	InetAddress peerAddr( sockets::getPeerAddr(connectedSocket->fd()) );
	char buf[32];
	snprintf( buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_ );
	std::string connName = name_ + buf;

	InetAddress localAddr( sockets::getLocalAddr(connectedSocket->fd()));
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	UdpConnectionPtr conn( new UdpConnection(loop_,
		connName, connectedSocket, nextConnId_, localAddr, peerAddr, kcpp::RoleTypeE::kCli ) );

	++nextConnId_;

	conn->setConnectionCallback( connectionCallback_ );
	conn->setMessageCallback( messageCallback_ );
	conn->setWriteCompleteCallback( writeCompleteCallback_ );
	conn->setCloseCallback(
		std::bind( &UdpClient::removeConnection, this, std::placeholders::_1 ) ); // FIXME: unsafe
	{
		base::MutexLockGuard lock( mutex_ );
		connection_ = conn;
	}
	conn->connectEstablished();
}

void UdpClient::removeConnection( const UdpConnectionPtr& conn )
{
	loop_->assertInLoopThread();
	assert( loop_ == conn->getLoop() );

	{
		base::MutexLockGuard lock( mutex_ );
		assert( connection_ == conn );
		connection_.reset();
	}

	loop_->queueInLoop( std::bind( &UdpConnection::connectDestroyed, conn ) );
	if ( retry_ && connect_ )
	{
		LOG_INFO << "UdpClient::connect[" << name_ << "] - Reconnecting to "
			<< connector_->serverAddress().toIpPort();
		connector_->restart();
	}
}




}
}
