#include "net/poller/EpollPoller.h"
#include "net/poller/PollPoller.h"
#include "net/poller/Poller.h"

namespace Miren
{
    namespace net
    {
                    
        Poller* Poller::newDefaultPoller(EventLoop* loop)
        {
            if(::getenv("MIREN_USE_POLL")) {
                return new PollPoller(loop);
            }
            else {
                return new EpollPoller(loop);
            }
        }

    } // namespace net
    
} // namespace Miren
