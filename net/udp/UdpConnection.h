#pragma once

#include "base/StringPiece.h"
#include "base/Types.h"
#include "base/Noncopyable.h"
#include "net/udp/UdpCallbacks.h"
#include "net/Callbacks.h"
#include "net/Buffer.h"
#include "net/sockets/InetAddress.h"
#include "net/timer/TimerId.h"

#include "third_party/kcpp/kcpp.h"

#include <memory>
#include <any>
// #include "any.h"


namespace Miren
{
	namespace net
	{

		class Channel;
		class EventLoop;
		class Socket;

		class UdpConnection : base::NonCopyable,	public std::enable_shared_from_this<UdpConnection>
		{
		public:
			/// Constructs a UdpConnection with a connected sockfd
			///
			/// User should not create this object.
			UdpConnection(EventLoop* loop,
				const std::string& name,
				Socket* connectedSocket,
				int ConnectionId,
				const InetAddress& localAddr,
				const InetAddress& peerAddr,
        const kcpp::RoleTypeE role);

			~UdpConnection();

			enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

			EventLoop* getLoop() const { return loop_; }
			const std::string& name() const { return name_; }
			const InetAddress& localAddress() const { return localAddr_; }
			const InetAddress& peerAddress() const { return peerAddr_; }

			bool connected() const { return state_ == kConnected; }
			bool disconnected() const { return state_ == kDisconnected; }

			bool canSend() const
			{ 
				if (kcpSession_)
					return kcpSession_->CheckCanSend();
				return true;
			}

			// void send(string&& message); // C++11

			void send(const void* data, int len,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable)
			{ send(base::StringPiece(static_cast<const char*>(data), len), transmitMode); }

			void send(const base::StringPiece& message,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable);
			//{ send(message.data(), message.size()); }

			// FIXME efficiency!!!
			void send(Buffer* buf,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable);
			//{ send(buf->peek(), static_cast<int>(buf->readableBytes())); }

			void shutdown(); // NOT thread safe, no simultaneous calling
							 // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
			void forceClose();
			//void forceCloseWithDelay( double seconds );

			// reading or not
			void startRead();
			void stopRead();
			bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

			void setConnectionCallback(const UdpConnectionCallback& cb) { connectionCallback_ = cb; }

			void setMessageCallback(const UdpMessageCallback& cb) { messageCallback_ = cb; }

			void setWriteCompleteCallback(const UdpWriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

			/// Internal use only.
			void setCloseCallback(const UdpCloseCallback& cb) { closeCallback_ = cb; }

			// called when TcpServer accepts a new connection
			void connectEstablished();   // should be called only once
												 // called when TcpServer has removed me from its map
			void connectDestroyed();  // should be called only once

			void setContext(const std::any& context) { context_ = context; }
		  const std::any& getContext() const { return context_; }
			std::any* getMutableContext() { return &context_; }

			int GetConnId() const { return connId_; }

		private:
			void kcpSend(const void* message, int len,
				kcpp::TransmitModeE transmitMode = kcpp::TransmitModeE::kReliable);

			void onKcpSessionConnection(std::deque<std::string>* pendingSendDataDeque);

			void KcpSessionUpdate();
			void DoSend(const void* message, int len);
			kcpp::UserInputData DoRecv();

			void handleRead(base::Timestamp receiveTime);
			void handleClose();
			void handleError();
			// void sendInLoop(string&& message);
			void sendInLoop(const base::StringPiece& message, kcpp::TransmitModeE transmitMode);
			void sendInLoop(const void* message, size_t len, kcpp::TransmitModeE transmitMode);
			// void shutdownAndForceCloseInLoop(double seconds);
			void forceCloseInLoop();
			void setState( StateE s ) { state_ = s; }
			const char* stateToString() const;
			void startReadInLoop();
			void stopReadInLoop();

			EventLoop* loop_;
			const std::string name_;
			StateE state_;  // FIXME: use atomic variable
			bool reading_;
			// we don't expose those classes to client.
			std::unique_ptr<Socket> socket_;
			std::unique_ptr<Channel> channel_;
			const InetAddress localAddr_;
			const InetAddress peerAddr_;
      
			UdpConnectionCallback connectionCallback_;
			UdpMessageCallback messageCallback_;
			UdpWriteCompleteCallback writeCompleteCallback_;
			UdpCloseCallback closeCallback_;

			// new
			int connId_;
			static const size_t kPacketBufSize = 1500;
			char packetBuf_[kPacketBufSize];
			std::any context_;

			Buffer inputBuffer_;
			kcpp::Buf kcpsessRcvBuf_;

			// kcp
			std::shared_ptr<kcpp::KcpSession> kcpSession_;
			TimerId curKcpsessUpTimerId_;
			//bool isCliKcpsessConned_;
		};

		typedef std::shared_ptr<UdpConnection> UdpConnectionPtr;

	}
}
