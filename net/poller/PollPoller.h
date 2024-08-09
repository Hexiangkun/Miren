#pragma once

#include "net/poller/Poller.h"

#include <vector>

struct pollfd;

namespace Miren
{
namespace net
{
    class PollPoller : public Poller
    {
    public:
        PollPoller(EventLoop* loop);
        ~PollPoller();

        base::Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

        void updateChannel(Channel* channel) override;

        void removeChannel(Channel* channel) override;
    private:
        void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    private:
        typedef std::vector<struct pollfd> PollFdList;
        PollFdList pollfds_;
    };
    
} // namespace net
} // namespace Miren::net
