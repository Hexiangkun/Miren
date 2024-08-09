#pragma once

#include "base/Timestamp.h"
#include "base/Types.h"
#include <functional>
#include <memory>

namespace Miren
{
  namespace net
  {
    class Buffer;
  } // namespace net
  
    namespace http
    {
        class HttpConnection;

        typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;
        typedef std::function<void ()> TimerCallback;
        typedef std::function<void (const HttpConnectionPtr&)> ConnectionCallback;
        typedef std::function<void (const HttpConnectionPtr&)> CloseCallback;
        typedef std::function<void (const HttpConnectionPtr&)> WriteCompleteCallback;
        typedef std::function<void (const HttpConnectionPtr&, size_t)> HighWaterMarkCallback;

        typedef std::function<void (const HttpConnectionPtr&, net::Buffer*, base::Timestamp)> MessageCallback;

        void defaultConnectionCallback(const HttpConnectionPtr& conn);
        void defaultMessageCallback(const HttpConnectionPtr& conn, net::Buffer* buffer, base::Timestamp recieveTime);
    }
}