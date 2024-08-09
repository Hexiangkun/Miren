#pragma once
#include "http/HttpTcpServer.h"
#include <functional>
#include <string>
namespace Miren
{

namespace http
{
class HttpSession;
class HttpRequest;
class HttpResponse;
class HttpServer 
{
 public:
  typedef std::function<void(std::shared_ptr<HttpSession>)> RequestCallback;
  HttpServer(net::EventLoop* loop,
             const net::InetAddress& listenAddr,
             const std::string& name,
             HttpTcpServer::Option option = HttpTcpServer::kNoReusePort);

  net::EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setRequestCallback(const RequestCallback& cb)
  {
    requestCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const HttpConnectionPtr& conn);
  void disConnection(const HttpConnectionPtr& conn);
  
  void onMessage(const HttpConnectionPtr& conn,
                 net::Buffer* buf,
                 base::Timestamp receiveTime);

  HttpTcpServer server_;
  RequestCallback requestCallback_;
  std::atomic_flag lock_;
  std::unordered_map<std::shared_ptr<HttpConnection>, std::shared_ptr<HttpSession>> httpSessions_;
};



}  // namespace http
}  // namespace Miren


