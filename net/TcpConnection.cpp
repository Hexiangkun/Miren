#include "net/TcpConnection.h"
#include "base/log/Logging.h"
#include "net/sockets/Socket.h"
#include "net/sockets/SocketsOps.h"
#include "net/Channel.h"
#include "net/Buffer.h"
#include "net/EventLoop.h"
#include "base/WeakCallback.h"
#include "base/ErrorInfo.h"
namespace Miren
{
    namespace net
    {
        // 默认连接建立或关闭时的回调函数
        void defaultConnectionCallback(const TcpConnectionPtr& conn)
        {
            LOG_TRACE << conn->localAddr().toIpPort() << " -> "
                        << conn->peerAddr().toIpPort() << " is "
                        << (conn->connected() ? "UP" : "DOWN");
        }
        //默认收到消息时的回调函数
        void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, base::Timestamp recieveTime)
        {
            buffer->retrieveAll();
        }

        TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                            const InetAddress& localAddr, const InetAddress& peerAddr)
                        :loop_(loop),
                        name_(name),
                        state_(kConnecting),        //正在连接
                        socket_(new Socket(sockfd)),
                        channel_(new Channel(loop, sockfd)),
                        localAddr_(localAddr),
                        peerAddr_(peerAddr),
                        highWarkMark_(64*1024*1024)
        {
            channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
            channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
            channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
            channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

            LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd = " << sockfd;

            socket_->setKeepAlive(true);
        }

        TcpConnection::~TcpConnection()
        {
            LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at "  << this 
                    << " fd= " << socket_->fd()
                    << " state= " << stateToString();
            assert(state_ == kDisconnected);
        }

        bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
        {
            return socket_->getTcpInfo(tcpi);
        }

        std::string TcpConnection::getTcpInfoString() const
        {
            char buf[1024];
            buf[0] = '\0';
            socket_->getTcpInfoString(buf, sizeof buf);
            return buf;
        }

        // 发送任意字节流
        void TcpConnection::send(const void* message, int len)
        {
            send(base::StringPiece(static_cast<const char*>(message), len));
        }

        void TcpConnection::send(const base::StringPiece& message)
        {
            if(state_ == kConnected) {
                if(loop_->isInLoopThread()) {
                    sendInLoop(message);        //在loop线程内，直接发送
                }
                else {                          //如果在别的线程发送数据，则将任务放入任务队列中
                    void (TcpConnection::*fp)(const base::StringPiece& message) = &TcpConnection::sendInLoop;
                    loop_->runInLoop(std::bind(fp, this, std::string(message)));
                }
            }
        }

        void TcpConnection::send(Buffer* buf)
        {
            if(state_ == kConnected) {
                if(loop_->isInLoopThread()) {
                    sendInLoop(buf->peek(), buf->readableBytes());
                    buf->retrieveAll();
                }
                else {
                    void (TcpConnection::*fp)(const base::StringPiece& message) = &TcpConnection::sendInLoop;
                    loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
                }
            }
        }

        void TcpConnection::sendInLoop(const base::StringPiece& message)
        {
            sendInLoop(message.data(), message.size());
        }

        void TcpConnection::sendInLoop(const void* message, size_t len)
        {
            loop_->assertInLoopThread();        //必须在loop线程内
            ssize_t nwrote = 0;
            size_t remaining = len;
            bool faultError = false;

            if(state_ == kDisconnected) {
                LOG_WARN << "disconnected, give up writing";
                return;
            }
            //如果输出缓冲区中没有数据，可以直接对fd写入数据
            if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
                nwrote = sockets::write(channel_->fd(), message, len);
                if(nwrote >= 0) {   
                    remaining = len - nwrote;   //剩余量
                    if(remaining == 0 && writeCompleteCallback_) {  //全部发送完毕
                        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                }
                else {//出现错误
                    nwrote = 0;
                    if(errno != EWOULDBLOCK) {
                        LOG_SYSERR << "TcpConnection::sendInLoop";
                        if(errno == EPIPE || errno == ECONNRESET) {
                            faultError = true;
                        }
                    }
                }
            }

            assert(remaining <= len);
            if(!faultError && remaining > 0) {
                size_t oldLen = outputBuffer_.readableBytes();
                if(oldLen + remaining >= highWarkMark_
                    && oldLen < highWarkMark_
                    && highWaterMarkCallback_) {
                    loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
                }
                //将未发送的数据放入输出缓冲区
                outputBuffer_.append(static_cast<const char*>(message)+nwrote, remaining);
                if(!channel_->isWriting()) {
                    channel_->enableWriting();
                }
            } 
        }

        void TcpConnection::shutdown()
        {
            if(state_ == kConnected) {  //正常连接情况下，关闭连接
                setState(kDisconnecting);
                loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
            }
        }

        void TcpConnection::shutdownInLoop()
        {
            loop_->assertInLoopThread();
            if(!channel_->isWriting()) {
                socket_->shutdownWrite();   //关闭写端，保证对方发送的数据能够完全接受
            }
        }

        void TcpConnection::forceClose()
        {
            if(state_ == kConnected || state_ == kDisconnecting) {
                setState(kDisconnected);
                loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
            }
        }

        void TcpConnection::forceCloseInLoop()
        {
            loop_->assertInLoopThread();
            if(state_ == kConnected || state_ == kDisconnecting) {
                handleClose();
            }
        }

        void TcpConnection::forceCloseWithDelay(double seconds)
        {
            if(state_ == kConnected || state_ == kDisconnecting) {
                setState(kDisconnected);
                loop_->runAfter(seconds, base::makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));
            }
        }

        void TcpConnection::setTcpNoDelay(bool on)
        {
            socket_->setTcpNoDelay(on);
        }


        void TcpConnection::startRead()
        {
            loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
        }

        void TcpConnection::stopRead()
        {
            loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
        }

        void TcpConnection::startReadInLoop()
        {
            loop_->assertInLoopThread();
            if(!reading_ || !channel_->isReading()) {
                channel_->enableReading();
                reading_ = true;
            }
        }

        void TcpConnection::stopReadInLoop()
        {
            loop_->assertInLoopThread();
            if(reading_ || channel_->isReading()) {
                channel_->disableReading();
                reading_ = false;
            }
        }

        void TcpConnection::connectEstablished()
        {
            loop_->assertInLoopThread();
            assert(state_ == kConnecting);
            setState(kConnected);
            channel_->tie(shared_from_this());
            channel_->enableReading();

            connectionCallback_(shared_from_this());
        }

        void TcpConnection::connectDestroyed()
        {
            loop_->assertInLoopThread();
            if(state_ == kConnected) {
                setState(kDisconnected);
                channel_->disableAll();

                connectionCallback_(shared_from_this());
            }
            channel_->remove();
        }

        const char* TcpConnection::stateToString() const
        {
            switch (state_)
            {
            case kDisconnected:
                return "kDisconnected";
            case kDisconnecting:
                return "kDisconnecting";
            case kConnected:
                return "kConnected";
            case kConnecting:
                return "kConnecting";
            default:
                return "unknown state";
            }
        }

        void TcpConnection::handleRead(base::Timestamp receiveTime)
        {
            loop_->assertInLoopThread();
            int savedErrno = 0;
            ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
            if(n > 0) {
                messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
            }
            else if(n == 0) {
                handleClose();  //连接关闭，执行关闭回调函数
            }
            else {
                errno = savedErrno;
                LOG_SYSERR << "TcpConnection::handleRead";
                handleError();
            }
        }

        void TcpConnection::handleWrite()
        {
            loop_->assertInLoopThread();
            if(channel_->isWriting()) {
                ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
                if(n > 0) {
                    outputBuffer_.retrieve(n);
                    if(outputBuffer_.readableBytes() == 0) { //所有数据发送完毕
                        channel_->disableWriting();     //停止监听写事件
                        if(writeCompleteCallback_) {
                            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                        }

                        if(state_ == kDisconnecting) {  //表示tcp处于半关闭
                            shutdownInLoop();   //数据全部发送完毕，这里需要册第关闭连接
                        }
                    }
                }
                else {
                    LOG_SYSERR << "TcpConnection::handleWrite";
                }
            }
            else {
                LOG_TRACE << "Connection fd = " << channel_->fd() << "is down, no more writing";
            }
        }

        void TcpConnection::handleClose()
        {
            loop_->assertInLoopThread();
            LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
            //处于正常和半关闭状态
            assert(state_ == kConnected || state_ == kDisconnecting);
            setState(kDisconnected);
            channel_->disableAll();

            TcpConnectionPtr guardThis(shared_from_this());
            connectionCallback_(guardThis); //执行用户关闭连接逻辑
            closeCallback_(guardThis);      //执行上层tcpserver注册的函数，执行removeConnection，在里面执行connectDestroyed
        }

        void TcpConnection::handleError()
        {
            int err = sockets::getSocketError(channel_->fd());
            LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << base::ErrorInfo::strerror_tl(err);
        }



        // void TcpConnection::send(ByteData* data) 
        // {
        //     if(state_ == kConnected) {
        //         if(loop_->isInLoopThread()) {
        //             sendInLoop(data);        //在loop线程内，直接发送
        //         }
        //         else {                          //如果在别的线程发送数据，则将任务放入任务队列中
        //             void (TcpConnection::*fp)(ByteData* message) = &TcpConnection::sendInLoop;
        //             loop_->runInLoop(std::bind(fp, this, data));
        //         }
        //     }
        // }

        // void TcpConnection::sendInLoop(ByteData* data) 
        // {

        // }
    } // namespace net
    
} // namespace Miren
