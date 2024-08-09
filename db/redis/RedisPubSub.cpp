#include "db/redis/RedisPubSub.h"
#include <thread>
#include "base/log/Logging.h"

namespace RedisConn
{
    RedisPubSub::~RedisPubSub() {
        if(publishContext_) {
            redisFree(publishContext_);
        }
        if(subscribeContext_) {
            redisFree(subscribeContext_);
        }
    }

    bool RedisPubSub::init(const char* host, unsigned int port, unsigned int initTimeoutMs)
    {
        bool ret = connectSvr(host, port, initTimeoutMs);
        if(!ret) {
            return false;
        }
        ret = connectSvr(&publishContext_, host, port, initTimeoutMs);
        if(!ret) {
            return false;
        }
        ret = connectSvr(&subscribeContext_, host, port, initTimeoutMs);
        if(!ret) {
            return false;
        }
        //在单独的线程中，监听通道上的时间，有消息给业务层进行上报
        std::thread t([&](){
           observer_channel_message();
        });
        t.detach();
        return true;
    }

    bool RedisPubSub::publish(int channel, const std::string& message) {
        redisReply* reply = (redisReply*) redisCommand(publishContext_, "PUBLISH %d %s", channel, message.c_str());
        if(reply == nullptr) {
            LOG_ERROR << "publish " << message << " failed!";
            return false;
        }
        freeReplyObject(reply);
        return true;
    }

    //subscribe命令本身会造成线程阻塞，等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    //通道消息的接收专门在observer_channel_message函数中独立线程中运行
    //只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    bool RedisPubSub::subscribe(int channel) {
        //将函数中的命令缓存到本地
        if(REDIS_ERR == redisAppendCommand(subscribeContext_, "SUBSCRIBE %d", channel)) {
            LOG_ERROR << "subscribe " << channel << " failed!";
            return false;
        }
        int done = 0;
        //将缓存区中的命令发送给redis server
        while (!done) {
            if(REDIS_ERR == redisBufferWrite(subscribeContext_, &done)) {
                LOG_ERROR << "redisBufferWrite error";
                return false;
            }
        }
        return true;
    }

    bool RedisPubSub::unsubscribe(int channel) {
        if(REDIS_ERR == redisAppendCommand(subscribeContext_, "UNSUBSCRIBE %d", channel)) {
            LOG_ERROR << "ubsubscribe " << channel << " failed";
            return false;
        }
        int done = 0;
        while(!done) {
            if(REDIS_ERR == redisBufferWrite(subscribeContext_, &done)) {
                LOG_ERROR << "redisBufferWrite " << channel << " failed" ;
                return false;
            }
        }
        return true;
    }

    //在独立的线程中接收订阅通道中的信息
    void RedisPubSub::observer_channel_message() {
        redisReply* reply = nullptr;
        while(REDIS_OK == redisGetReply(subscribeContext_, (void**)&reply)) {
            // 订阅收到的消息是一个带三元素的数组
            /**
             * 1) "message"
               2) "mychannel"
               3) "Hello"
             * */
            if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr) {
                //给业务层上报通道上发生的消息
                notifyMessageHandler_(atoi(reply->element[1]->str), reply->element[2]->str);
            }
            freeReplyObject(reply);
        }
    }

    void RedisPubSub::initNotifyHandler(std::function<void(int, std::string)> fn) {
        this->notifyMessageHandler_ = std::move(fn);
    }

}