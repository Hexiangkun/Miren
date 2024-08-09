#pragma once

#include "base/Noncopyable.h"
#include "base/Timestamp.h"

#include <functional>
#include <memory>

namespace Miren::net
{
    class EventLoop;
    /*
    负责事件的分发:
    最起码得拥有的数据成员有，分发哪个文件描述符上的事件，即上面数据成员中的fd_，
    其次得知道该poller监控该文件描述符上的哪些事件，对应events_，
    接着就是当该文件描述符就绪之后其上发生了哪些事件对应上面的revents。
    知道了发生了什么事件，要达到事件分发的功能你总得有各个事件的处理回调函数，对应上面的各种callback。
    最后该Channel由哪个loop_监控并处理的loop_
    */
    /*
    channel对应一个fd
    channel封装了fd对应的操作：add、mod、del
    注册fd的事件：read、write、none
    使用回调函数方法：可读、可写、错误、关闭
    channel的生命周期不属于持有它的EventLoop，也不属于Poller，一般属于TcpConnection、Acceptor、Connector等
    */
    class Channel : base::NonCopyable
    {
    public:
        typedef std::function<void()> EventCallback;        //事件回调函数
        typedef std::function<void(base::Timestamp)> ReadEventCallback; //读事件回调函数

        Channel(EventLoop* loop, int fd);
        ~Channel();

        // 处理回调事件
        void handleEvent(base::Timestamp recieveTime);
        void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
        void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
        void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
        void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

        /// 将此channel绑定到由 shared_ptr 管理的所有者对象，
        /// 防止所有者对象在 handleEvent 中被销毁。
        void tie(const std::shared_ptr<void>&);

        EventLoop* ownerLoop() { return loop_; }
        int fd() const { return fd_; }
        int events() const { return events_; }
        int revents() const { return revents_; }
        void set_revents(int revent) { revents_ = revent; }

        bool isNoneEvent() const { return events_ == kNoneEvent; }
        bool isWriting() const { return events_ & kWriteEvent; }
        bool isReading() const { return events_ & kReadEvent; }

        void enableReading() { events_ |= kReadEvent; update(); }
        void disableReading() { events_ &= ~kReadEvent; update(); }
        void enableWriting() { events_ |= kWriteEvent; update(); }
        void disableWriting() { events_ &= ~kWriteEvent; update(); }
        void disableAll() { events_ = kNoneEvent; update(); }
    
        // 提供给poller使用
        int index() { return index_; }
        void set_index(int idx) { index_ = idx; }

        // debug
        std::string reventsToString() const;
        std::string eventsToString() const;

        void doNotLogHup() { logHup_ = false; }
        // 将channel从EventLoop中移除，也就是从poller中停止监听该fd
        void remove();
    private:
        void update();  //通过调用loop中的函数，改变fd在epoll中监听的事件
        void handleEventWithGuard(base::Timestamp receiveTime);//在临界区处理事件，会根据revents触发的事件来分别决定调用哪些回调 
        static std::string eventsToString(int fd, int ev);
    private:
        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        EventLoop* loop_;   // 属于哪一个Reactor
        const int fd_;      // 关联的fd
        int events_;        // 用户关心的IO事件
        int revents_;       // 活跃的事件
        int index_;         //PollPoller中才使用
        bool logHup_;
        
        std::weak_ptr<void> tie_;
        bool tied_;

        bool eventHandling_;    //是否正在处理事件
        bool addToLoop_;        //是否添加了channel到loop中

        ReadEventCallback readCallback_;    // 读回调
        EventCallback writeCallback_;       // 写回调
        EventCallback closeCallback_;       // 定义如何关闭连接
        EventCallback errorCallback_;       // 错误处理
    };
}