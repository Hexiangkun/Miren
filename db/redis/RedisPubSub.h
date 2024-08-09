#pragma once

#include <hiredis/hiredis.h>
#include <string>
#include <functional>

#include "db/redis/RedisConn.h"

namespace RedisConn
{
    static const std::string noStr = "nil";
    class RedisPubSub : public RedisConnection
    {
    public:
        RedisPubSub() : publishContext_(nullptr), subscribeContext_(nullptr), notifyMessageHandler_(nullptr) {}
        ~RedisPubSub() ;

        bool init(const char* host, unsigned int port, unsigned int timeoutMs = 1500);

        //向redis指定的通道channel发布消息
        bool publish(int channel, const std::string& message);

        //向redis指定通道订阅消息
        bool subscribe(int channel);

        //向redis指定通道取消订阅消息
        bool unsubscribe(int channel);

        //在独立线程中接受订阅通道中的消息
        void observer_channel_message();

        //初始化向业务层上报通道消息的回调函数
        void initNotifyHandler(std::function<void(int, std::string)> fn);

    private:
        //负责publish消息
        redisContext* publishContext_;
        //负责subscribe消息
        redisContext* subscribeContext_;
        //回调操作，收到订阅的消息，给service层上报
        std::function<void(int, std::string)> notifyMessageHandler_;
    };
}