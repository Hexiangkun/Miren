#pragma once

#include <string>
#include <vector>

class OfflineMsgModel {

public:
    // 存储用户的离线消息
    bool insert(const int usrID, const std::string &msg);
    // 删除用户的离线消息
    bool remove(const int usrID);
    // 将数据库的离线消息输出到一个队列中去
    bool query(const int usrID, std::vector<std::string> &vec);
};

