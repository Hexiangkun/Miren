
#pragma once

#include <functional>
#include <map>
#include "base/Noncopyable.h"
#include "net/sockets/InetAddress.h"
#include "net/sockets/Socket.h"
#include "net/Channel.h"
#include "net/Buffer.h"

namespace Miren
{
	namespace net
	{

		class EventLoop;
		class InetAddress;
		
		class UdpAcceptor : base::NonCopyable
		{
		public:
			typedef std::function<void(Socket*, const InetAddress&)> NewConnectionCallback;
			typedef std::function<void(const InetAddress&)> NewConnectorCallback;

			UdpAcceptor( EventLoop* loop, const InetAddress& listenAddr, bool reuseport );
			~UdpAcceptor();

			void setNewConnectionCallback( const NewConnectionCallback& cb )	{ newConnectionCallback_ = cb; }
			void setNewConnectorCallback(const NewConnectorCallback& cb) { newConnectorCallback_ = cb; }

			bool listenning() const { return listenning_; }
			void listen();

			uint16_t GetListenPort() const { return listenPort_; }
			EventLoop* getLoop() const { return loop_; }
		private:
			void handleRead();

			EventLoop* loop_;
			Socket acceptSocket_;
			Channel acceptChannel_;
			NewConnectionCallback newConnectionCallback_;
			NewConnectorCallback newConnectorCallback_;
			bool listenning_;
			uint16_t listenPort_;
			InetAddress listenAddr_;

			static const size_t krecvfromBufSize = 1500;
			char recvfromBuf_[krecvfromBufSize];
		};

	}
}
