#pragma once

#include "base/Timestamp.h"
#include "rpc/protobuf/ProtobufCodecLite.h"

namespace Miren
{
namespace net
{
		class Buffer;
		class TcpConnection;
		typedef std::shared_ptr<Miren::net::TcpConnection> TcpConnectionPtr;

	namespace rpc
	{

		class RpcMessage;
		typedef std::shared_ptr<RpcMessage> RpcMessagePtr;

		extern const char rpctag[5];

		typedef ProtobufCodecLiteT<RpcMessage, rpctag> RpcCodec;

	} // namespace rpc
} // namespace net
	
} // namespace Miren
