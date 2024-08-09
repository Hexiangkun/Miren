#include "http/HttpServer.h"
#include "http/HttpSession.h"
#include "base/log/Logging.h"
namespace Miren
{
namespace http
{
namespace detail
{

void defaultRequestCallback(std::shared_ptr<HttpSession>)
{
  // resp->setStatusCode(HttpResponse::k404NotFound);
  // resp->setStatusMessage("Not Found");
  // resp->setCloseConnection(true);
}

void writecb(const HttpConnectionPtr& conn) 
{
  LOG_INFO << "write complete";
}

}  // namespace detail

HttpServer::HttpServer(net::EventLoop* loop,
                       const net::InetAddress& listenAddr,
                       const std::string& name,
                       HttpTcpServer::Option option)
  : server_(loop, listenAddr, name, option),
    requestCallback_(detail::defaultRequestCallback),
    lock_(ATOMIC_FLAG_INIT)
{
  server_.setWriteCompleteCallback(std::bind(&detail::writecb, std::placeholders::_1));
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name()
    << "] starts listenning on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const HttpConnectionPtr& conn)
{
  if (conn->connected())
  {
    std::shared_ptr<HttpSession> session(new HttpSession(conn, requestCallback_));
    while(lock_.test_and_set(std::memory_order_acquire)) {}
    httpSessions_[conn] = session;
    lock_.clear(std::memory_order_release);
  } 
  else {
    LOG_INFO << "disconnected";
    disConnection(conn);
  }
}

void HttpServer::disConnection(const HttpConnectionPtr& conn)
{
  auto iter = httpSessions_.find(conn);
  if(iter != httpSessions_.end()) {
    while(lock_.test_and_set(std::memory_order_acquire)) {}
    httpSessions_.erase(conn);
    lock_.clear(std::memory_order_release);
  }
}

void HttpServer::onMessage(const HttpConnectionPtr& conn,
                           net::Buffer* buf,
                           base::Timestamp receiveTime)
{
  std::shared_ptr<HttpSession> httpSession = httpSessions_[conn];
  LOG_INFO << buf->toStringPiece().size();
  if (!httpSession->parse(buf, receiveTime))
  {
    std::string res = "HTTP/1.1 400 Bad Request\r\n\r\n";
    ByteData* data = new ByteData();
    data->addDataZeroCopy(res);
    conn->send(data);
    conn->shutdown();
  }
  LOG_INFO  << "parse success";
  httpSession->handleParsedMessage();
  LOG_INFO <<  "handle success";
}
}
    
} // namespace Miren
