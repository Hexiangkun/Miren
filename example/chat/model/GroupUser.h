#pragma once
#include "example/chat/model/User.h"

//用户在群里的属性，多了一个角色
class GroupUser : public User
{
public:
    void setRole(const std::string& role) {
        _role = role;
    }

    const std::string getRole() {
        return _role;
    }

private:
    std::string _role;  //改用户在群组里的角色：管理员或者普通成员
};
