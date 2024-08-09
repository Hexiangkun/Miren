#pragma once

#include "base/Noncopyable.h"
#include "net/sockets/InetAddress.h"

#include <functional>
#include <memory>

namespace Miren
{
namespace net
{
  class Channel;
  class EventLoop;

  /*
  ***Connector用来非阻塞主动发起发起连接,带有重连功能.
  ***该类不单独使用，而是放于TcpClient中使用
  */
  class Connector : base::NonCopyable, public std::enable_shared_from_this<Connector>
  {
  public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    void start();     // can be called in any thread
    void stop();      // can be called in any thread
    void restart();   // must be called in loop thread

    const InetAddress& serverAddress() const { return serverAddr_; }
  private:
    //未连接状态,正在连接(中间状态),已连接,
    enum States { kDisconnected, kConnecting, kConnected }; 
    static const int kMaxRetryDelayMs = 30 * 1000;  //最大重试延迟
    static const int kInitRetryDelayMs = 500;       //初始化重试延迟

    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();
  private:
    EventLoop* loop_;
    InetAddress serverAddr_;
    bool connect_;
    States state_;
    std::unique_ptr<Channel> channel_;              //Connector所对应的Channel
    NewConnectionCallback newConnectionCallback_;   //连接成功回调函数
    int retryDelayMs_;                              //重连延迟时间(单位ms)
  };
} // namespace net
  
} // namespace Miren
