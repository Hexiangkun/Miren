#include "net/Connector.h"

#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/sockets/SocketsOps.h"
#include "base/log/Logging.h"
#include "base/ErrorInfo.h"
#include <errno.h>


namespace Miren
{
namespace net
{
  const int Connector::kMaxRetryDelayMs;

  //构造函数初始化了I/O线程，服务器地址，并设置为未连接状态以及初始化了重连延时时间
  Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
      :loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
  {
    LOG_DEBUG << "Connector ctor[" << this << "]";
  }

  Connector::~Connector()
  {
    LOG_DEBUG << "Connector dtor[" << this << "]";
    assert(!channel_);
  }


  void Connector::start()
  {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
  }

  //在当前IO线程中建立连接
  void Connector::startInLoop()
  {
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if(connect_) {
      connect();    // 开始建立连接
    }
    else {
      LOG_DEBUG << "do not connect";
    }
  }

  void Connector::restart()
  {
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
  }

  void Connector::stop()
  {
    connect_ = false;
    loop_->runInLoop(std::bind(&Connector::stopInLoop, this));
  }

  void Connector::stopInLoop()
  {
    loop_->assertInLoopThread();
    if(state_ == kConnecting) {
      setState(kDisconnected);
      int sockfd = removeAndResetChannel();
      retry(sockfd);
    }
  }

  void Connector::connect()
  {
    int sockfd = sockets::createNoblockingOrDie(serverAddr_.family());
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
  //以下是非阻塞socket connect的流程
  //如果不用非阻塞,但是如果对端服务器由于某些问题无法连接，
  //那么每一个客户端发起的connect都会要等待75才会返回
  //通过检查错误码来判断的状态和决定如何操作
    switch (savedErrno)
    {
    case 0:
    //这个错误表示连接正在进行中。在非阻塞socket上调用connect时可能会返回这个错误，表示连接尚未完成，需要稍后再次检查连接状态
    case EINPROGRESS:
    //表示系统调用被中断。这通常发生在信号中断了一个系统调用的执行过程中，通常是可以重新尝试的
    case EINTR:
    //表示socket已经连接到一个目标地址。
    case EISCONN:
      connecting(sockfd);
      break;
    //表示系统资源暂时不可用。在非阻塞socket上，可能会在连接操作中返回这个错误，表示当前无法完成连接操作，但稍后可能可以重试。
    case EAGAIN:
    //表示目标地址已经被其他socket占用，无法再次使用。
    case EADDRINUSE:
    //表示请求的地址不可用。可能是由于地址不在本地网络接口上，或者其他网络配置问题导致的。
    case EADDRNOTAVAIL:
    //表示目标地址拒绝连接请求。
    case ECONNREFUSED:
    //表示网络不可达。
    case ENETUNREACH:
      retry(sockfd);
      break;
    //表示没有权限执行操作。
    case EACCES:
    //表示操作不被允许。通常
    case EPERM:
    //表示地址族不被协议支持。
    case EAFNOSUPPORT:
    //表示操作已经在进行中
    case EALREADY:
    //表示文件描述符无效。
    case EBADF:
    //表示提供的地址不正确。
    case EFAULT:
    //表示尝试在一个不是socket的文件描述符上执行socket操作
    case ENOTSOCK:
      LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);
      break;
    default:
      LOG_SYSERR << "unexpected error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);
      break;
    }
  }


//如果连接成功,即errno为EINPROGRESS、EINTR、EISCONN
//连接成功就是更改连接状态+设置各种回调函数+加入poller关注可写事件
  void Connector::connecting(int sockfd)
  {
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd)); //Channel与sockfd关联
    //设置可写回调函数，这时候如果socket没有错误，sockfd就处于可写状态;但是非阻塞,错误时也会有可写状态,因为要判断
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
  }

  int Connector::removeAndResetChannel()
  {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
  }

  void Connector::resetChannel()
  {
    channel_.reset();
  }

//非阻塞connect,可写不一定表示已经建立连接
//因此要获取err来判断
  void Connector::handleWrite()
  {
    LOG_TRACE << "Connector::handleWrite " << state_;
    if(state_ == kConnecting) {
      int sockfd = removeAndResetChannel();//移除channel。Connector中的channel只管理建立连接阶段。连接建立后，交给TcpConnection管理
      int err = sockets::getSocketError(sockfd);//sockfd可写不一定建立了连接，这里通过此再次判断一下
      if(err) {
        LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << base::ErrorInfo::strerror_tl(err);
        retry(sockfd);
      }
      else if(sockets::isSelfConnect(sockfd)) {//判断是否是自连接（源端IP/PORT=目的端IP/PORT），原因见书籍P328
        LOG_WARN << "Connector::handleWrite - self connect";
        retry(sockfd);
      }
      else {//表示确实连接成功
        setState(kConnected);
        if(connect_) {
          newConnectionCallback_(sockfd);//执行TcpClient.cc的构造函数指定的连接回调函数
        }
        else {
          sockets::close(sockfd);
        }
      }
    }
    else {
      assert(state_ == kDisconnected);
    }
  }

  void Connector::handleError()
  {
    LOG_ERROR << "Connector::handleError state=" << state_;
    if(state_ == kConnecting) {
      int sockfd = removeAndResetChannel();
      int err = sockets::getSocketError(sockfd);
      LOG_TRACE << "SO_ERROR = " << err << " " << base::ErrorInfo::strerror_tl(err);
      retry(sockfd);
    }
  }

  void Connector::retry(int sockfd)
  {
    sockets::close(sockfd);//关闭原有的sockfd。每次尝试连接，都需要使用新sockfd
    setState(kDisconnected);
    if(connect_) {
      LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
          << " in " << retryDelayMs_ << " milliseconds.";
      //隔一段时间后重连，重新启用startInLoop
      loop_->runAfter(retryDelayMs_/1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
      retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else {
      LOG_DEBUG << "do not connect";
    }
  }

} // namespace net
 
} // namespace Miren
