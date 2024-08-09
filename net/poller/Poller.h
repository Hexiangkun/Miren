#pragma once

#include "base/Noncopyable.h"
#include "base/Timestamp.h"
#include "net/EventLoop.h"
#include <map>
#include <vector>

namespace Miren
{
    namespace net
    {
        class Channel;

        /*
        Poller类是IO复用类的基类。由EpollPoller和PollPoller两个类继承
        每个EventLoop都持有一个Poller子类对象
        但是EventLoop和Poller都不持有Channel对象，仅仅包含Channel的指针数组
        */
        class Poller : base::NonCopyable
        {
        public:
            typedef std::vector<Channel*> ChannelList;
            Poller(EventLoop* loop);
            virtual ~Poller();

            virtual base::Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
            // 更新监听fd的事件
            virtual void updateChannel(Channel* channel) = 0;
            //删除监听fd
            virtual void removeChannel(Channel* channel) = 0;
            // 判断poll子类对象是否监听该fd
            virtual bool hasChannel(Channel* channel) const;
            //用于产生一个Poller子类对象
            static Poller* newDefaultPoller(EventLoop* loop);
            //断言没有跨线程
            void assertInLoopThread() const
            {
                ownerLoop_->assertInLoopThread();
            }
        protected:
            // fd 到 Channel的映射
            typedef std::map<int, Channel*> ChannelMap;
            ChannelMap channelsMap_;
        private:
            EventLoop* ownerLoop_;  //poller对象所属的EventLoop
        };
    } // namespace net
    
} // namespace Miren
