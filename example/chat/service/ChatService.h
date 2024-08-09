//
// Created by 37496 on 2024/3/16.
//

#ifndef WEBSERVER_CHATSERVICE_H
#define WEBSERVER_CHATSERVICE_H

#include "base/Timestamp.h"
#include "net/TcpConnection.h"
#include "third_party/nlohmann/json.hpp"
#include "db/redis/RedisPubSub.h"
#include "example/chat/control/UserModel.h"
#include "example/chat/control/OfflineMsgModel.h"
#include "example/chat/control/FriendModel.h"
#include "example/chat/control/GroupModel.h"
#include <functional>
#include <mutex>

typedef std::function<void(const Miren::net::TcpConnectionPtr& , nlohmann::json& , Miren::base::Timestamp)> msgHandler;


class ChatService
{
public:
    //单例模式
    static ChatService* getInstance();
    //暴露给chatServer接口，用int整型匹配对应的处理函数
    msgHandler getHandler(int msgId);

    //登录
    void login(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);
    //注册
    void regis(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);

    //处理客户端异常退出
    void clientCloseException(const Miren::net::TcpConnectionPtr& connection);
    //处理服务器宕机
    void serverCloseException();

    void oneChat(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);

    void addFriend(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);

    void createGroup(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);

    void addToGroup(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);

    void groupChat(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);

    void logout(const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp);


private:
    ChatService();

    std::unordered_map<int, msgHandler > _msgHandlerMap;

    std::unordered_map<int, Miren::net::TcpConnectionPtr > _userConnMap;

    std::mutex _connMtx;

    UserModel _userModel;

    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    RedisConn::RedisPubSub _redisModel;

    //订阅频道有更新，获取消息
    void redisNotifyHandler(int ,std::string);
};


#endif //WEBSERVER_CHATSERVICE_H
