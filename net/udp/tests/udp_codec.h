#pragma once

#include "base/log/Logging.h"
#include "net/Buffer.h"
#include "net/sockets/Endian.h"
#include "net/udp/UdpConnection.h"

class UdpLengthHeaderCodec{
public:

	typedef std::function<void(const Miren::net::UdpConnectionPtr&,
		const std::string& message,
		Miren::base::Timestamp)> UdpStringMessageCallback;

	explicit UdpLengthHeaderCodec(const UdpStringMessageCallback& cb)
		: udp_messageCallback_(cb)
	{}

	void onMessage(const Miren::net::UdpConnectionPtr& conn,
		Miren::net::Buffer* buf,
		Miren::base::Timestamp receiveTime)
	{
		//LOG_INFO << "into UdpLengthHeaderCodec::onMessage";

		while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
		{
			// FIXME: use Buffer::peekInt32()
			const void* data = buf->peek();
			int32_t be32 = *static_cast<const int32_t*>(data); // SIGBUS
			const int32_t len = Miren::net::sockets::networkToHost32(be32);
			if (len > 65536 || len < 0)
			{
				LOG_ERROR << "Invalid length " << len;
				conn->shutdown();  // FIXME: disable reading
				break;
			}
			else if (buf->readableBytes() >= len + kHeaderLen)
			{
				buf->retrieve(kHeaderLen);
				std::string message(buf->peek(), len);
				udp_messageCallback_(conn, message, receiveTime);
				//LOG_INFO << "into UdpLengthHeaderCodec::onMessage messageCallback_(conn, message, receiveTime);";
				buf->retrieve(len);
			}
			else
			{
				break;
			}
		}
	}

	// FIXME: UdpConnectionPtr
	void send(Miren::net::UdpConnection* conn,
		const Miren::base::StringPiece& message)
	{
		Miren::net::Buffer buf;
		buf.append(message.data(), message.size());
		int32_t len = static_cast<int32_t>(message.size());
		int32_t be32 = Miren::net::sockets::hostToNetwork32(len);
		buf.prepend(&be32, sizeof be32);
		conn->send(&buf);
	}

private:

	UdpStringMessageCallback udp_messageCallback_;
	const static size_t kHeaderLen = sizeof(int32_t);
};
