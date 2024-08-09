#pragma once

#include "base/Timestamp.h"

#include <functional>
#include <memory>

namespace Miren
{
	namespace net
	{

		// All client visible callbacks go here.

		class Buffer;
		class UdpConnection;
		typedef std::shared_ptr<UdpConnection> UdpConnectionPtr;
		typedef std::function<void()> UdpTimerCallback;
		typedef std::function<void( const UdpConnectionPtr& )> UdpConnectionCallback;
		typedef std::function<void( const UdpConnectionPtr& )> UdpCloseCallback;
		typedef std::function<void( const UdpConnectionPtr& )> UdpWriteCompleteCallback;
		typedef std::function<void( const UdpConnectionPtr&, size_t )> UdpHighWaterMarkCallback;

		//// the data has been read to (buf, len)
		//typedef std::function<void(const UdpConnectionPtr&,
		//	char*,
		//	size_t,
		//	Timestamp)> UdpMessageCallback;

		//void UdpDefaultConnectionCallback(const UdpConnectionPtr& conn);
		//void UdpDefaultMessageCallback(const UdpConnectionPtr& conn,
		//	char* buf,
		//	size_t bufBytes,
		//	Timestamp recvTime);

		// the data has been read to (buf, len)
		typedef std::function<void(const UdpConnectionPtr&,
			Buffer*,
			base::Timestamp)> UdpMessageCallback;

		void UdpDefaultConnectionCallback(const UdpConnectionPtr& conn);
		void UdpDefaultMessageCallback(const UdpConnectionPtr& conn,
			Buffer* buffer,
			base::Timestamp recvTime);
	}
}

