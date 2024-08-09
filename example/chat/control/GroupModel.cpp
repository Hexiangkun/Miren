//
// Created by 37496 on 2024/3/15.
//

#include "example/chat/control/GroupModel.h"
#include "db/mysql/MysqlConnection.h"
#include <cstring>
#pragma GCC diagnostic ignored "-Wshadow"
//创建群组
bool GroupModel::createGroup(Group &group) {
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getGName().c_str(), group.getGName().c_str());

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        if (mysql.ExecuteNonQuery(sql))
        {
            SqlConn::MySqlDataReader* rd = mysql.ExecuteReader("select id from allgroup where groupname= ? and groupdesc = ?", group.getGName(), group.getGName());
            while(rd->Read()) {
                int id;
                rd->GetValues(id);
                group.setID(id);
            }
            return true;
        }
    }
    return false;
}

bool GroupModel::addUser(int userId, int groupId, const std::string &role) {
    // 1.组装sql语句
    char sql[1024] = {0};
    // 检查groupId的合法性
    sprintf(sql, "select id from allgroup where id = %d", groupId);

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if(rd && rd->Read()) {
            ::memset(sql, 0, sizeof sql);
            sprintf(sql, "insert into groupuser values(%d, %d, '%s')",
                    groupId, userId, role.c_str());
            return mysql.ExecuteNonQuery(sql) > 0;
        }
    }
    return false;
}

Group GroupModel::query(int groupId) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select id, groupname, groupdesc from allgroup where id = %d", groupId);
    Group group;
    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        while(rd->Read()) {
            int id;
            std::string name;
            std::string desc;
            rd->GetValues(id, name, desc);
            group.setID(id);
            group.setGName(name);
            group.setGDesc(desc);
            return group;
        }
    }
    return group;
}

//查询用户所在群组信息
void GroupModel::queryUserAllGroup(int userid ,std::vector<Group>& groups) {
    /*
    1. 先根据userid在groupuser表中查询出该用户所属的群组信息
    2. 在根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid=%d",
            userid);

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        while(rd->Read()) {
            Group group;
            int id;
            std::string name;
            std::string desc;
            rd->GetValues(id, name, desc);
            group.setID(id);
            group.setGName(name);
            group.setGDesc(desc);
            groups.push_back(std::move(group));
        }
    }

    for(Group& g: groups) {
        memset(sql, 0, sizeof sql);
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join groupuser b on b.userid = a.id where b.groupid = %d", g.getID());
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if(rd!= nullptr) {
            while(rd->Read()) {
                int id;
                std::string name;
                std::string state;
                std::string role;
                rd->GetValues(id, name, state, role);
                GroupUser groupUser;
                groupUser.setId(id);
                groupUser.setName(name);
                groupUser.setState(state);
                groupUser.setRole(role);
                g.getUsers().push_back(groupUser);
            }
        }
    }
}


//获取一个组内的所有成员的id
bool GroupModel::queryOneGroup(int userid, int groupid, std::vector<int>& userIds) {
    char sql[1024];
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);
    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);

        if(rd) {
            while (rd->Read()) {
                int id;
                rd->GetValues(id);
                userIds.push_back(id);
            }
            return true;
        }
    }
    return false;
}

//获取一个组内所有成员的信息
bool GroupModel::queryOneGroup(int usrid, int gID, std::vector<GroupUser> &usrVec) {
    // 组装sql语句
    char sql[1024];
    sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid = %d",
            gID);
    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if (rd != nullptr)
        {
            while (rd->Read())
            {
                GroupUser gu;
                int id;
                std::string name, state, role;
                rd->GetValues(id, name, state, role);
                gu.setId(id);
                gu.setName(name);
                gu.setState(state);
                gu.setRole(role);
                usrVec.push_back(gu);
            }
            return true;
        }
    }
    return false;
}


// 检查gid是否为服务器拥有的群
bool GroupModel::checkGroup(int gID)
{
    // 组装sql语句
    char sql[1024];
    sprintf(sql, "select id from allgroup where id = %d", gID);

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if (rd != nullptr && rd->Read())
        {
            return true;
        }
    }
    return false;
}