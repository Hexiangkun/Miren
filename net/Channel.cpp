#include "net/Channel.h"
#include "net/EventLoop.h"
#include "base/log/Logging.h"
#include <poll.h>
#include <sstream>
namespace Miren
{
    namespace net
    {
        const int Channel::kNoneEvent = 0;                  // 无事件
        const int Channel::kReadEvent = POLLIN | POLLPRI;   // 读事件。PRI表示紧急数据，如socket的带外数据
        const int Channel::kWriteEvent = POLLOUT;           // 写事件

        Channel::Channel(EventLoop* loop, int fd)
            :loop_(loop),
            fd_(fd),
            events_(0),
            revents_(0),
            index_(-1),
            logHup_(false),
            tied_(false),
            eventHandling_(false),
            addToLoop_(false)
        {

        }

        //析构函数，必须确定loop中已经移除了该fd，或者epoll中删除了该fd
        Channel::~Channel()
        {
            assert(!eventHandling_);
            assert(!addToLoop_);
            if(loop_->isInLoopThread()) {
                assert(!loop_->hasChannel(this));
            }
        }

        void Channel::handleEvent(base::Timestamp recieveTime)
        {
            std::shared_ptr<void> guard;
            if(tied_) {
                //提升为shared_ptr,保证处理事件时不会析构
                guard = tie_.lock();
                if(guard) {
                    handleEventWithGuard(recieveTime);
                }
            }
            else {
                handleEventWithGuard(recieveTime);
            }
        }

        void Channel::tie(const std::shared_ptr<void>& obj)
        {
            tie_ = obj;
            tied_ = true;
        }

        std::string Channel::reventsToString() const
        {
            return eventsToString(fd_, revents_);
        }

        std::string Channel::eventsToString() const
        {
            return eventsToString(fd_, events_);
        }

        // 确保该fd不再监听任何事件，即先调用disableAll
        void Channel::remove()
        {
            assert(isNoneEvent());
            addToLoop_ = false;
            loop_->removeChannel(this);
        }

        //调用loop函数，改变fd在poller中监听的事件，
        // channel -> loop -> poller
        void Channel::update()
        {
            addToLoop_ = true;
            loop_->updateChannel(this);
        }

        void Channel::handleEventWithGuard(base::Timestamp receiveTime)
        {
            eventHandling_ = true;
            LOG_TRACE << reventsToString();

            //连接断开POLLHUP只可能在output产生(man poll)，读的时候不会产生！所以要判断
            if((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
                if(logHup_) {
                    LOG_WARN << "fd = " << fd_ << ", Channel::handleEvent() POLLHUP";
                }
                if(closeCallback_) {
                    closeCallback_();
                }
            }

            if(revents_ & POLLNVAL) {   //表示fd未打开
                LOG_WARN << "fd = " << fd_ << ", Channel::handleEvent() POLLNVAL";
            }

            if(revents_ & (POLLERR | POLLNVAL)) {
                if(errorCallback_) {
                    errorCallback_();
                }
            }
            /*
            readCallback_作用：
            1.TImerQueue用来读timerfd
            2.EventLoop用来读eventfd  
            3.TcpServer/Acceptor用来读listening socket
            4.TcpConnection用来读普通TCP socket
            */

            if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) { //可读事件或者对方关闭连接
                if(readCallback_) {
                    readCallback_(receiveTime);
                }
            }

            if(revents_ & POLLOUT) {    //可写事件产生
                if(writeCallback_) {
                    writeCallback_();
                }
            }
            eventHandling_ = false;
        }

        std::string Channel::eventsToString(int fd, int ev)
        {
            std::ostringstream oss;
            oss << fd << ": ";
            if(ev & POLLIN) {
                oss << "POLLIN ";
            }
            if(ev & POLLPRI) {
                oss << "POLLPRI ";
            }
            if(ev & POLLOUT) {
                oss << "POLLOUT ";
            }
            if(ev & POLLHUP) {
                oss << "POLLHUP ";
            }
            if(ev & POLLRDHUP) {
                oss << "POLLRDHUP ";
            }
            if(ev & POLLERR) {
                oss << "POLLERR ";
            }
            if(ev & POLLNVAL) {
                oss << "POLLNVAL ";
            }
            return oss.str();
        }

    } // namespace net
    
} // namespace Miren
