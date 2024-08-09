
#include "example/chat/model/Group.h"
#include "example/chat/model/GroupUser.h"
#include <vector>

class GroupModel {
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组里添加一个用户
    bool addUser(int userId, int groupId, const std::string& role);

    //查询群组的描述信息
    Group query(int groupId);

    // 查询用户所在群组信息
    void queryUserAllGroup(int userid,std::vector<Group>& );

    // 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其它成员群发消息
    bool queryOneGroup(int userid, int groupid, std::vector<int>& userIds);
    bool queryOneGroup(int usrid, int gID, std::vector<GroupUser> &usrVec);

    bool checkGroup(int gId);
};
