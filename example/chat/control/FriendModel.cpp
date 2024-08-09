//
// Created by 37496 on 2024/3/15.
//

#include "example/chat/control/FriendModel.h"
#include "db/mysql/MysqlConnection.h"
#include "third_party/nlohmann/json.hpp"

#include <cstring>

bool FriendModel::insert(int id, int fid) {
    char sql[1024];
    sprintf(sql, "select id from user where id=%d", fid);

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if(rd!= nullptr && rd->Read()) {
            ::memset(sql, 0, sizeof sql);
            sprintf(sql, "insert into friend(userid, friendid) values(%d, %d)", id, fid);
            return mysql.ExecuteNonQuery(sql);
        }
    }
    return false;
}

void FriendModel::query(int userId, std::vector<std::string> &result) {
    char sql[1024];
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userId);
    //查询userid的好友信息

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if(rd != nullptr) {
            nlohmann::json js;
            while(rd->Read()) {
                int id;
                std::string name, state;
                rd->GetValues(id, name, state);
                js["id"] = id;
                js["name"] = name;
                js["state"] = state;
                result.push_back(js.dump());
            }
        }

        // 还需要再查一次，把userID作为friendID查
        ::memset(sql, 0, sizeof sql);
        sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.userid = a.id where b.friendid = %d", userId);
        rd = mysql.ExecuteReader(sql);


        if(rd!= nullptr) {
            nlohmann::json js;

            while(rd->Read()) {
                int id;
                std::string name, state;
                rd->GetValues(id, name, state);
                js["id"] = id;
                js["name"] = name;
                js["state"] = state;
                result.push_back(js.dump());
            }

        }
    }
}