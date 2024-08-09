#include "net/poller/Poller.h"
#include "net/Channel.h"

namespace Miren::net
{
    Poller::Poller(EventLoop* loop) : ownerLoop_(loop)
    {

    }

    Poller::~Poller() = default;

    bool Poller::hasChannel(Channel* channel) const
    {
        assertInLoopThread();
        ChannelMap::const_iterator it = channelsMap_.find(channel->fd());
        return it != channelsMap_.end() && it->second == channel;
    }
}