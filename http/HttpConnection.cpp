#include "http/HttpConnection.h"
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
    namespace http
    {
        

        void defaultConnectionCallback(const HttpConnectionPtr& conn)
        {
            LOG_TRACE << conn->localAddr().toIpPort() << " -> "
                        << conn->peerAddr().toIpPort() << " is "
                        << (conn->connected() ? "UP" : "DOWN");
        }

        void defaultMessageCallback(const HttpConnectionPtr& conn, net::Buffer* buffer, base::Timestamp recieveTime)
        {
            buffer->retrieveAll();
        }

        HttpConnection::HttpConnection(net::EventLoop* loop, const std::string& name, int sockfd,
                            const net::InetAddress& localAddr, const net::InetAddress& peerAddr)
                        :loop_(loop),
                        name_(name),
                        state_(kConnecting),
                        socket_(new net::Socket(sockfd)),
                        channel_(new net::Channel(loop, sockfd)),
                        localAddr_(localAddr),
                        peerAddr_(peerAddr),
                        highWarkMark_(64*1024*1024)
        {
            channel_->setReadCallback(std::bind(&HttpConnection::handleRead, this, std::placeholders::_1));
            channel_->setWriteCallback(std::bind(&HttpConnection::handleWrite, this));
            channel_->setCloseCallback(std::bind(&HttpConnection::handleClose, this));
            channel_->setErrorCallback(std::bind(&HttpConnection::handleError, this));

            LOG_DEBUG << "HttpConnection::ctor[" << name_ << "] at " << this << " fd = " << sockfd;

            socket_->setKeepAlive(true);
        }

        HttpConnection::~HttpConnection()
        {
            LOG_DEBUG << "HttpConnection::dtor[" << name_ << "] at "  << this 
                    << " fd= " << socket_->fd()
                    << " state= " << stateToString();
            assert(state_ == kDisconnected);
            while (send_datas_.size()) {
                ByteData* data = send_datas_.front();
                send_datas_.pop();
                delete data;
            }
        }

        bool HttpConnection::getTcpInfo(struct tcp_info* tcpi) const
        {
            return socket_->getTcpInfo(tcpi);
        }

        std::string HttpConnection::getTcpInfoString() const
        {
            char buf[1024];
            buf[0] = '\0';
            socket_->getTcpInfoString(buf, sizeof buf);
            return buf;
        }

        void HttpConnection::sendInLoop(const void* message, size_t len)
        {
            loop_->assertInLoopThread();
            ssize_t nwrote = 0;
            size_t remaining = len;
            bool faultError = false;

            if(state_ == kDisconnected) {
                LOG_WARN << "disconnected, give up writing";
                return;
            }

            if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
                nwrote = net::sockets::write(channel_->fd(), message, len);
                if(nwrote >= 0) {
                    remaining = len - nwrote;
                    if(remaining == 0 && writeCompleteCallback_) {
                        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                }
                else {
                    nwrote = 0;
                    if(errno != EWOULDBLOCK) {
                        LOG_SYSERR << "HttpConnection::sendInLoop";
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
                outputBuffer_.append(static_cast<const char*>(message)+nwrote, remaining);
                if(!channel_->isWriting()) {
                    channel_->enableWriting();
                }
            } 
        }

        void HttpConnection::shutdown()
        {
            if(state_ == kConnected) {
                setState(kDisconnecting);
                loop_->runInLoop(std::bind(&HttpConnection::shutdownInLoop, this));
            }
        }

        void HttpConnection::shutdownInLoop()
        {
            loop_->assertInLoopThread();
            if(!channel_->isWriting()) {
                socket_->shutdownWrite();
            }
        }

        void HttpConnection::forceClose()
        {
            if(state_ == kConnected || state_ == kDisconnecting) {
                setState(kDisconnected);
                loop_->queueInLoop(std::bind(&HttpConnection::forceCloseInLoop, shared_from_this()));
            }
        }

        void HttpConnection::forceCloseInLoop()
        {
            loop_->assertInLoopThread();
            if(state_ == kConnected || state_ == kDisconnecting) {
                handleClose();
            }
        }

        void HttpConnection::forceCloseWithDelay(double seconds)
        {
            if(state_ == kConnected || state_ == kDisconnecting) {
                setState(kDisconnected);
                loop_->runAfter(seconds, base::makeWeakCallback(shared_from_this(), &HttpConnection::forceClose));
            }
        }




        void HttpConnection::setTcpNoDelay(bool on)
        {
            socket_->setTcpNoDelay(on);
        }


        void HttpConnection::startRead()
        {
            loop_->runInLoop(std::bind(&HttpConnection::startReadInLoop, this));
        }

        void HttpConnection::stopRead()
        {
            loop_->runInLoop(std::bind(&HttpConnection::stopReadInLoop, this));
        }

        void HttpConnection::startReadInLoop()
        {
            loop_->assertInLoopThread();
            if(!reading_ || !channel_->isReading()) {
                channel_->enableReading();
                reading_ = true;
            }
        }

        void HttpConnection::stopReadInLoop()
        {
            loop_->assertInLoopThread();
            if(reading_ || channel_->isReading()) {
                channel_->disableReading();
                reading_ = false;
            }
        }

        void HttpConnection::connectEstablished()
        {
            loop_->assertInLoopThread();
            assert(state_ == kConnecting);
            setState(kConnected);
            channel_->tie(shared_from_this());
            channel_->enableReading();

            connectionCallback_(shared_from_this());
        }

        void HttpConnection::connectDestroyed()
        {
            loop_->assertInLoopThread();
            if(state_ == kConnected) {
                setState(kDisconnected);
                channel_->disableAll();

                connectionCallback_(shared_from_this());
            }
            channel_->remove();
        }

        const char* HttpConnection::stateToString() const
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

        void HttpConnection::handleRead(base::Timestamp receiveTime)
        {
            loop_->assertInLoopThread();
            int savedErrno = 0;
            ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
            LOG_INFO << n;
            if(n > 0) {
                messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
            }
            else if(n == 0) {
                handleClose();
            }
            else {
                errno = savedErrno;
                LOG_SYSERR << "HttpConnection::handleRead";
                handleError();
            }
        }

        void HttpConnection::handleClose()
        {
            loop_->assertInLoopThread();
            LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
            assert(state_ == kConnected || state_ == kDisconnecting);
            setState(kDisconnected);
            channel_->disableAll();

            HttpConnectionPtr guardThis(shared_from_this());
            connectionCallback_(guardThis);
            closeCallback_(guardThis);
        }

        void HttpConnection::handleError()
        {
            int err = net::sockets::getSocketError(channel_->fd());
            LOG_ERROR << "HttpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << base::ErrorInfo::strerror_tl(err);
        }
       
       // -------------------
        void HttpConnection::handleWrite()
        {
            loop_->assertInLoopThread();
            if(channel_->isWriting()) {
                ByteData* data = send_datas_.front();
                ssize_t n = data->writev(channel_->fd());
                
                if(n > 0) {
                    if(!data->remain()) {
                        send_datas_.pop();
                        delete data;
                    }
                    if(send_datas_.size() == 0) {
                        channel_->disableWriting();
                        if(writeCompleteCallback_) {
                            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                        }

                        if(state_ == kDisconnecting) {
                            shutdownInLoop();
                        }
                    }
                }
                else {
                    LOG_SYSERR << "HttpConnection::HandleWrite";
                }
            }
            else {
                LOG_TRACE << "Connection fd = " << channel_->fd() << "is down, no more writing";
            }
        }

        void HttpConnection::send(const void* data, size_t size) 
        {
            ByteData* bdata = new ByteData();
            bdata->addDataZeroCopy(data, size);
            send(bdata);
        }

        void HttpConnection::send(ByteData* bdata) 
        {
            if(state_ == kConnected) {
                if(loop_->isInLoopThread()) {
                    sendInLoop(bdata);
                }
                else {
                    void (HttpConnection::*fp)(ByteData*) = &HttpConnection::sendInLoop;
                    loop_->queueInLoop(std::bind(fp, this, bdata));
                }
            }
        }

        void HttpConnection::sendInLoop(ByteData* data) 
        {
            loop_->assertInLoopThread();
            ssize_t nwrote = 0;
            bool faultError = false;
            bool flag = false;
            if(state_ == kDisconnected) {
                LOG_WARN << "disconnected, give up writing";
                return;
            }

            if(!channel_->isWriting() && send_datas_.size() == 0) {

                nwrote = data->writev(channel_->fd());

                if(nwrote >= 0) {
                    if(!data->remain()) {
                        if(writeCompleteCallback_) {
                            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                        }
                        delete data;
                        flag = true;
                        data = nullptr;     //必须滞空，否则出现bug
                    }
                }
                else {
                    nwrote = 0;
                    if(errno != EWOULDBLOCK) {
                        LOG_SYSERR << "HttpConnection::sendInLoop";
                        if(errno == EPIPE || errno == ECONNRESET) {
                            faultError = true;
                        }
                    }
                }
            }
            

            if(!faultError && !flag && data != nullptr && data->remain()) {
                data->copyDataIfNeed();
                send_datas_.push(data);
                // if(oldLen + remaining >= highWarkMark_
                //     && oldLen < highWarkMark_
                //     && highWaterMarkCallback_) {
                //     loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
                // }
                if(!channel_->isWriting()) {
                    channel_->enableWriting();
                }
            } 
        }


    } // namespace http
    
} // namespace Miren
