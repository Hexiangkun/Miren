//
// Created by 37496 on 2024/3/16.
//

#include "example/chat/service/ChatService.h"
#include "example/chat/public.h"
#include "base/log/Logging.h"

#pragma GCC diagnostic ignored "-Wshadow"

using namespace std::placeholders;
ChatService::ChatService() {
    _msgHandlerMap.insert({LOG_IN, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG, std::bind(&ChatService::regis, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_TO_GROUP, std::bind(&ChatService::addToGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOG_OUT, std::bind(&ChatService::logout, this, _1, _2, _3)});

    if(_redisModel.init("127.0.0.1", 6379)) {
        _redisModel.initNotifyHandler(std::bind(&ChatService::redisNotifyHandler, this, _1, _2));
    }
}

ChatService *ChatService::getInstance() {
    //饿汉模式，线程安全的
    static ChatService service;
    return &service;
}

msgHandler ChatService::getHandler(int msgId) {
    if(_msgHandlerMap.count(msgId)) {
        return _msgHandlerMap[msgId];
    }
    else {
        return [=](const Miren::net::TcpConnectionPtr& connection, nlohmann::json& js, Miren::base::Timestamp stamp) {
            LOG_ERROR <<  "msgId: " << msgId << "can't find in the msgHandler!" ;
        };
    }
}

void ChatService::login(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                        Miren::base::Timestamp stamp) {
    /*
登录分为四种情况：
    1. 找不到账号
    2. 账号正确，但是密码错误
    3. 账号正确，密码正确，未登录用户
    4. 账号正确，密码正确，已登录用户
*/
    int id = js[ID].get<int>();
    std::string password = js[PWD];
    //查询用户是否存在
    User user = _userModel.query(id);

    //创建返回消息json
    nlohmann::json response;

    response[MSG_ID] = LOG_IN_MSG_ACK;

    if(user.getId() == -1) {    //找不到账号
        response[ERRNO] = 1;
        response[MSG] = "can not find the id";
    }
    else if(user.getId() == id) {
        if(user.getPassword() != password) {    //找到账号，密码不对
            response[ERRNO] = 2;
            response[MSG] = "incorrect password!";
        }
        else if(user.getState() == "online") {  //已经登录
            response[ERRNO] = 3;
            response[MSG] = "already online! can not login again!";
        }
        else {      //未登录
            //登录成功
            response[ERRNO] = 0;
            response[ID] = user.getId();
            response[NAME] = user.getName();

            user.setState("online");    //更细数据库用户的状态
            _userModel.updateState(user);

            {   //添加连接
                std::unique_lock<std::mutex> lock(_connMtx);
                _userConnMap.insert({user.getId(), connection});
            }

            //拉取离线消息
            std::vector<std::string> offmsg;
            if(_offlineMsgModel.query(user.getId(), offmsg)) {
                _offlineMsgModel.remove(user.getId());
            }

            if(!offmsg.empty()) {
                response[OFF_MSG] = offmsg;
            }

            //显示自己的好友
            std::vector<std::string > friends;
            _friendModel.query(user.getId(), friends);
            if(!friends.empty()) {
                response[FRIEND_LIST] = friends;
            }

            //拉取群
            std::vector<Group> groups;
            _groupModel.queryUserAllGroup(user.getId(), groups);

            if(!groups.empty()) {
                std::vector<std::string> groupInfos;
                for(Group& g: groups) {
                    nlohmann::json oneGroupInfo;
                    oneGroupInfo[GROUP_ID] = g.getID();
                    oneGroupInfo[GROUP_NAME] = g.getGName();
                    oneGroupInfo[GROUP_DESC] = g.getGDesc();
                    std::vector<std::string> groupMem;
                    for(GroupUser& mem : g.getUsers()) {
                        nlohmann::json info;
                        info[ID] = mem.getId();
                        info[NAME] = mem.getName();
                        info[STATE] = mem.getState();
                        info[GROUP_ROLE] = mem.getRole();
                        groupMem.push_back(info.dump());
                    }
                    oneGroupInfo[GROUP_MEMS] = groupMem;
                    groupInfos.push_back(oneGroupInfo.dump());
                }
                response[GROUP_LIST] = groupInfos;
            }

            //在redis中订阅频道--》自身的id。如果别人向我这个id频道发了消息，我需要被提醒然后感知消息
            _redisModel.subscribe(user.getId());

            //服务器推送
            //当客户端登录成功后，应该告知当前用户的所有好友，“我以上线”
            std::string tt = getCurrentTime();
            {
                std::unique_lock<std::mutex> lock(_connMtx);
                for(auto& f:friends) {
                    nlohmann::json fInfo = nlohmann::json::parse(f);
                    int id = fInfo[ID].get<int>();

                    //推送的消息
                    nlohmann::json pushMsg;
                    pushMsg[MSG_ID] = FRIEND_ONLINE;
                    pushMsg[ID] = user.getId();
                    pushMsg[TIME] = tt;
                    if(_userConnMap.count(id)) {
                        _userConnMap[id]->send(pushMsg.dump());
                    }//此服务器上没有这个好友的socket，需要判断是否在其他服务器上
                    else if(_userModel.query(id).getState() == "online") {
                        //用户在线，将消息发送到消息队列上
                        _redisModel.publish(id, pushMsg.dump());
                    }
                }
            }
        }
    }
    //向客户端发送登录状态
    connection->send(response.dump()+"\n");
}

void ChatService::regis(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                        Miren::base::Timestamp stamp) {
    std::string name = js[NAME];
    std::string password = js[PWD];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);

    nlohmann::json response;
    response[MSG_ID] = REG_MSG_ACK;
    if(state) {
        //注册成功，向客户端发送注册成功消息
        response[ERRNO] = 0;
        response[ID] = user.getId();
    }
    else {
        response[ERRNO] = -1;
    }
    connection->send(response.dump() + "\n");
}

//客户端断开连接，服务端不会收到json，业务层面不会做到对客户端异常退出的检测
//TcpConnection会感知到，
void ChatService::clientCloseException(const Miren::net::TcpConnectionPtr &connection) {
    User user;
    {
        std::unique_lock<std::mutex> lock(_connMtx);
        for(auto& it : _userConnMap) {
            if(it.second == connection) {
                user.setId(it.first);
                break;
            }
        }
    }
    if(user.getId()!=-1) {
        {
            std::unique_lock<std::mutex> lock(_connMtx);
            _userConnMap.erase(user.getId());
        }
        _userModel.updateState(user);
    }
}

//处理服务器宕机
void ChatService::serverCloseException() {
    _userModel.offlineAll();
}

void ChatService::oneChat(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                          Miren::base::Timestamp stamp) {
    /*
一对一聊天，消息格式：
{
    MSG_ID: ONE_CHAT,
    ID: the sender,
    FRIEND_ID: the reciver,
    MSG: the chat message
    TIME: cur time
}
*/
    /*
    json           json
消息发送方 -----> 服务器 -----> 消息接收方
 */
    int destId = js[FRIEND_ID].get<int>();
    {
        std::unique_lock<std::mutex> lock(_connMtx);
        auto iter = _userConnMap.find(destId);
        if(iter!=_userConnMap.end()) {
            LOG_INFO << "接收方在线";
            iter->second->send(js.dump());
            return;
        }
    }
    //当前客户在其他服务器上，把消息发送到redis
    if(_userModel.query(destId).getState() == "online") {
        _redisModel.publish(destId, js.dump());
    }
    else {
        _offlineMsgModel.insert(destId, js.dump());
        LOG_INFO << "离线消息保存完毕";
    }

}

/**
 * MSG_ID:
 * ID:
 * FRIEND_ID:
 * */

void ChatService::addFriend(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                            Miren::base::Timestamp stamp) {
    // 获取自身的id和好友的id
    int id = js[ID].get<int>();
    int friendId = js[FRIEND_ID].get<int>();

    //插入
    nlohmann::json response;
    response[MSG_ID] = ADD_FRIEND_ACK;
    response[TIME] = getCurrentTime();

    //调用friendmodel修改friend表
    if(_friendModel.insert(id, friendId)) {
        response[ERRNO] = 0;
        response[MSG] = "add friend successed!";

        //返回添加的好友的user信息
        User friendInfo = _userModel.query(friendId);
        response[ID] = friendInfo.getId();
        response[NAME] = friendInfo.getName();
        response[STATE] = friendInfo.getState();
    }
    else {
        response[ERRNO] = -1;
        response[MSG] = "add friend failed!";
    }
    connection->send(response.dump());
}

void ChatService::createGroup(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                             Miren::base::Timestamp stamp) {
    int usrId = js[ID].get<int>();
    std::string gname = js[GROUP_NAME].get<std::string>();
    std::string gdesc = js[GROUP_DESC].get<std::string>();
    Group group(-1, gname, gdesc);

    nlohmann::json response;
    response[MSG_ID] = CREATE_GROUP_ACK;
    response[TIME] = getCurrentTime();

    if(_groupModel.createGroup(group)) {
        _groupModel.addUser(usrId, group.getID(), "creator");
        response[GROUP_ID] = group.getID();
        response[GROUP_NAME] = group.getGName();
        response[GROUP_DESC] = group.getGDesc();
        response[MSG] = "create group success!";
        response[ERRNO] = 0;
    }
    else {
        response[MSG] = "create group failed!";
        response[ERRNO] = -1;
    }
    connection->send(response.dump());
}

//添加用户至群组
void ChatService::addToGroup(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                             Miren::base::Timestamp stamp) {
    int userId = js[ID].get<int>();
    int groupId = js[GROUP_ID].get<int>();

    nlohmann::json response;
    response[MSG_ID] = ADD_TO_GROUP_ACK;
    response[TIME] = getCurrentTime();
    Group group = _groupModel.query(groupId);
    if(_groupModel.addUser(userId, groupId, "normal") && group.getID() != -1) {

        response[GROUP_ID] = groupId;
        response[GROUP_NAME] = group.getGName();
        response[GROUP_DESC] = group.getGDesc();
        response[MSG] = "add to group success!";
        response[ERRNO] = 0;

        //获取这个群完整信息
        std::vector<GroupUser> tmp;
        _groupModel.queryOneGroup(userId, groupId, tmp);
        std::vector<std::string> members;
        for(auto& it : tmp) {
            nlohmann::json mem;
            mem[ID] = it.getId();
            mem[NAME] = it.getName();
            mem[STATE] = it.getState();
            mem[GROUP_ROLE] = it.getRole();
            members.push_back(mem.dump());
        }
        response[GROUP_MEMS] = members;
    }
    else {
        response[MSG] = "add to group failed!";
        response[ERRNO] = -1;
    }

    connection->send(response.dump());
}

void ChatService::groupChat(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                            Miren::base::Timestamp stamp) {
    int userId = js[ID].get<int>();
    int groupId = js[GROUP_ID].get<int>();

    std::string msg = js[MSG].get<std::string>();

    std::vector<int> groupMem;

    if(_groupModel.queryOneGroup(userId, groupId, groupMem)) {
        std::unique_lock<std::mutex> lock(_connMtx);

        for(int memid : groupMem) {
            if(_userConnMap.count(memid)) {
                _userConnMap[memid]->send(js.dump());
            }
            else if(_userModel.query(memid).getState() == "online") {
                _redisModel.publish(memid, js.dump());
            }
            else {
                _offlineMsgModel.insert(memid, js.dump());
            }
        }
    }
}


void ChatService::logout(const Miren::net::TcpConnectionPtr &connection, nlohmann::json &js,
                         Miren::base::Timestamp stamp) {
    User user;
    user.setId(js[ID].get<int>());
    user.setState("offline");
    //删除操作，上锁
    {
        std::unique_lock<std::mutex> lock(_connMtx);
        _userConnMap.erase(user.getId());
    }
    //修改数据库
    _userModel.updateState(user);
    //redis取消订阅
    _redisModel.unsubscribe(user.getId());

    //服务器向该用户所有好友推送，此用户已下线
    std::vector<std::string> fvec;
    _friendModel.query(user.getId(), fvec);
    if(!fvec.empty()) {
        {
            std::unique_lock<std::mutex> lock(_connMtx);
            std::string tt = getCurrentTime();

            for(std::string& str : fvec) {
                nlohmann::json finfo = nlohmann::json::parse(str);
                int id = finfo[ID].get<int>();

                nlohmann::json pushMsg;
                pushMsg[MSG_ID] = FRIEND_OFFLINE;
                pushMsg[ID]  = user.getId();
                pushMsg[TIME] = tt;
                if(_userConnMap.count(id)) {
                    _userConnMap[id]->send(pushMsg.dump());
                }
                else if(_userModel.query(id).getState() == "online") {
                    _redisModel.publish(id, pushMsg.dump());
                }
            }
        }
    }
}


void ChatService::redisNotifyHandler(int id, std::string msg) {
    std::unique_lock<std::mutex> lock(_connMtx);
    auto iter = _userConnMap.find(id);
    if(iter!=_userConnMap.end()) {
        iter->second->send(msg);
        return;
    }
    _offlineMsgModel.insert(id, msg);
}
