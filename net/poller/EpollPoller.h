#pragma once

#include "net/poller/Poller.h"
#include <vector>

struct epoll_event;


namespace Miren
{
    namespace net
    {
        class EpollPoller : public Poller
        {
        public:
            EpollPoller(EventLoop* loop);
            ~EpollPoller();

            base::Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

            void updateChannel(Channel* channel) override;

            void removeChannel(Channel* channel) override;

        private:
            static const char* operationToString(int op);
            
            void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

            void update(int operation, Channel* channel);
            
        private:
            static const int kInitEventListSize = 16;
            typedef std::vector<struct epoll_event> EventList;

            int epollfd_;
            EventList events_;
        };
    } // namespace net
    
} // namespace Miren
