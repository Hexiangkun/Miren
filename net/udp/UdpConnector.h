#pragma once

#include "base/Noncopyable.h"
#include "net/sockets/InetAddress.h"
#include "net/sockets/Socket.h"

#include <functional>
#include <memory>

namespace Miren
{
namespace net
{
  class Channel;
  class EventLoop;

  class UdpConnector : base::NonCopyable, public std::enable_shared_from_this<UdpConnector>
  {
  public:
    typedef std::function<void(int)> NewConnectionCallback;
    typedef std::function<void(Socket*)> NewUdpConnectionCallback;


    UdpConnector( EventLoop* loop, const InetAddress& serverAddr, const InetAddress& localAddr);
    ~UdpConnector();

    void setNewConnectionCallback( const NewConnectionCallback& cb ) { newConnectionCallback_ = cb; }
    void setNewUdpConnectionCallback( const NewUdpConnectionCallback& cb ) { newUdpConnectionCallback_ = cb; }

    void start();  // can be called in any thread
    void restart();  // must be called in loop thread
    void stop();  // can be called in any thread

    const InetAddress& serverAddress() const { return serverAddr_; }

    /////////// new : for UDP
    //const Socket& GetConnectSocket() const { return connectSocket_; }

  private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState( States s ) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd );
    void handleWrite();
    void handleError();
    void retry( int sockfd );
    int removeAndResetChannel();
    void resetChannel();

    /////////// new : for UDP
    void connected(Socket* connectedSocket);
    //Socket connectSocket_;


    EventLoop* loop_;
    InetAddress serverAddr_;
    InetAddress localAddr_;
    bool connect_; // atomic
    States state_;  // FIXME: use atomic variable
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    NewUdpConnectionCallback newUdpConnectionCallback_;

    int retryDelayMs_;

  };

}
}
