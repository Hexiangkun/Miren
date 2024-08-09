#pragma once

#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include "base/Noncopyable.h"

namespace Miren
{
namespace net
{

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer : Miren::base::NonCopyable
{
 public:
  typedef std::function<void (const HttpRequest&,
                                HttpResponse*)> HttpCallback;  //http回调类型

  HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const std::string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);  //构造函数

  ~HttpServer();  // force out-line dtor, for scoped_ptr members.

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 base::Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr&, const HttpRequest&);//根据http请求，进行相应处理

  TcpServer server_;  //http服务器也是一个Tcp服务器，所以包含一个TcpServer
  HttpCallback httpCallback_;  //在处理http请求时(即调用onRequest)的过程中回调此函数，对请求进行具体的处理。
};

}
}
