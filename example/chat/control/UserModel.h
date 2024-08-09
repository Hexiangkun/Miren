#pragma once

#include "example/chat/model/User.h"

//User表增删改查类

class UserModel
{
public:
    //User表增加方法
    bool insert(User& user);

    //用id查询User表中的数据
    User query(int id);

    //更新User的数据登录状态
    bool updateState(User& user);

    //服务器宕机，下线所有用户
    bool offlineAll();
};