//
// Created by 37496 on 2024/3/15.
//

#ifndef WEBSERVER_FRIENDMODEL_H
#define WEBSERVER_FRIENDMODEL_H

#include <vector>
#include <string>

class FriendModel {
public:
    //添加一个好友
    bool insert(int id, int fid);

    //返回自身id对应的好友列表
    void query(int userId, std::vector<std::string>& result);
};


#endif //WEBSERVER_FRIENDMODEL_H
