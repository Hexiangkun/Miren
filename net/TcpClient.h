#pragma once

#include "base/thread/Mutex.h"
#include "net/TcpConnection.h"

namespace Miren
{
namespace net
{
  class Connector;
  typedef std::shared_ptr<Connector> ConnectorPtr;
  class TcpClient : base::NonCopyable
  {
  public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const { return connection_; }
    EventLoop* loop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }
    const std::string name() const { return name_; }

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }
  
  private: 
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);
  private:
    EventLoop* loop_;
    ConnectorPtr connector_;
    const std::string name_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    bool retry_;
    bool connect_;
    int nextConnId_;
    mutable base::MutexLock mutex_;
    TcpConnectionPtr connection_ GUARDED_BY(mutex_);
  };
} // namespace net
 
} // namespace Miren
