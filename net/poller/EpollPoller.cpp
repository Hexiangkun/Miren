#include "net/poller/EpollPoller.h"
#include "net/Channel.h"
#include "base/log/Logging.h"
#include <sys/epoll.h>
#include <poll.h>
#include <unistd.h>
namespace Miren
{
    namespace net
    {
        static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
        static_assert(EPOLLPRI == POLLPRI,  "epoll uses same flag values as poll");
        static_assert(EPOLLOUT == POLLOUT,  "epoll uses same flag values as poll");
        static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
        static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");
        static_assert(EPOLLERR == POLLERR,  "epoll uses same flag values as poll");

        namespace
        {
            const int kNew = -1;
            const int kAdded = 1;
            const int kDeleted = 2;
        }


        EpollPoller::EpollPoller(EventLoop* loop)
                :Poller(loop), 
                epollfd_(epoll_create1(EPOLL_CLOEXEC)),
                events_(kInitEventListSize)
        {
            if(epollfd_ < 0) {
                LOG_SYSFATAL << "EpollPoller::EpollPoller";
            }
        }
        EpollPoller::~EpollPoller()
        {
            ::close(epollfd_);
        }

        base::Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) 
        {
            LOG_TRACE << "fd total count: " << channelsMap_.size();
            int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
            int savedErrno = errno;
            base::Timestamp now(base::Timestamp::now());

            if(numEvents > 0) {
                LOG_TRACE << numEvents << " events happened";
                fillActiveChannels(numEvents, activeChannels);
                if(base::implicit_cast<size_t>(numEvents) == events_.size()) {
                    events_.resize(events_.size() * 2);
                }
            }
            else if(numEvents == 0) {
                LOG_TRACE << "nothing happened";
            }
            else {
                if(savedErrno != EINTR) {
                    errno = savedErrno;
                    LOG_SYSERR << "EpollPoller::poll()";
                }
            }
            return now;
        }

        void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
        {
            assert(base::implicit_cast<size_t>(numEvents) <= events_.size());
            for(int i=0; i<numEvents; i++) {
                Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            
            #ifndef NDEBUG
                int fd = channel->fd();
                ChannelMap::const_iterator it = channelsMap_.find(fd);
                assert(it != channelsMap_.end());
                assert(it->second == channel);
            #endif
                
                channel->set_revents(events_[i].events);
                activeChannels->push_back(channel);
            }
        }


        void EpollPoller::updateChannel(Channel* channel)
        {
            Poller::assertInLoopThread();
            const int index = channel->index();
            LOG_TRACE << "fd = " << channel->fd() << ", events = " << channel->events() << ", index = " << index;
            if(index == kNew || index == kDeleted) {
                int fd = channel->fd();
                if(index == kNew) {
                    assert(channelsMap_.find(fd) == channelsMap_.end());
                    channelsMap_[fd] = channel;
                }
                else {
                    assert(channelsMap_.find(fd) != channelsMap_.end());
                    assert(channelsMap_[fd] == channel);
                }

                channel->set_index(kAdded);
                update(EPOLL_CTL_ADD, channel);
            }
            else {
                int fd = channel->fd();
                (void)fd;

                assert(channelsMap_.find(fd) != channelsMap_.end());
                assert(channelsMap_[fd] == channel);
                assert(index == kAdded);
                if(channel->isNoneEvent()) {
                    update(EPOLL_CTL_DEL, channel);
                    channel->set_index(kDeleted);
                }
                else {
                    update(EPOLL_CTL_MOD, channel);
                }
            }
        }

        void EpollPoller::removeChannel(Channel* channel)
        {
            Poller::assertInLoopThread();
            int fd = channel->fd();

            LOG_TRACE << "fd = " << fd;
            assert(channelsMap_.find(fd) != channelsMap_.end());
            assert(channelsMap_[fd] == channel);
            assert(channel->isNoneEvent());

            int index = channel->index();
            assert(index == kAdded || index == kDeleted);
            size_t n = channelsMap_.erase(fd);
            (void)n;
            assert(n == 1);

            if(index == kAdded) {
                update(EPOLL_CTL_DEL, channel);
            }
            channel->set_index(kNew);
        }

        void EpollPoller::update(int operation, Channel* channel)
        {
            struct epoll_event event;
            base::MemoryZero(&event, sizeof event);
            event.events = channel->events();
            event.data.ptr = channel;
            int fd = channel->fd();

            LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
                      << ", fd = " << fd << ", event = { " << channel->eventsToString() << " }";
            
            if(::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
                if(operation == EPOLL_CTL_DEL) {
                    LOG_SYSERR << "epoll_ctl op = " << operationToString(operation) << ", fd = " << fd;
                }
                else {
                    LOG_SYSFATAL << "epoll_ctl op = " << operationToString(operation) << ", fd = " << fd;
                }
            }
        }

        const char* EpollPoller::operationToString(int op)
        {
            switch(op)
            {
                case EPOLL_CTL_ADD :
                    return "ADD";
                case EPOLL_CTL_DEL:
                    return "DEL";
                case EPOLL_CTL_MOD:
                    return "MOD";
                default:
                    assert(false && "ERROR operation");
                    return "unknown operation";
            }
        }
            
    } // namespace net
    
} // namespace Miren
