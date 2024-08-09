//
// Created by 37496 on 2024/3/15.
//

#include "example/chat/control/UserModel.h"
#include "db/mysql/MysqlConnection.h"
#pragma GCC diagnostic ignored "-Wshadow"
bool UserModel::insert(User& user) {
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());


    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        if (mysql.ExecuteNonQuery(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            int id = -1;
            SqlConn::MySqlDataReader *rd = mysql.ExecuteReader("select id from user where name = ?", user.getName());
            while (rd->Read())
            {
                rd->GetValues(id);
            }
            user.setId(id);
            return true;
        }
    }

    return false;
}


User UserModel::query(int id)
{
// 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        while(rd->Read()) {
            int id;
            std::string name;
            std::string passward;
            std::string state;
            rd->GetValues(id, name, passward, state);
            User user(id, name, passward, state);
            return user;
        }
    }
    return User();
}

bool UserModel::updateState(User& user)
{
// 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        if (mysql.ExecuteNonQuery(sql))
        {
            return true;
        }
    }
    return false;
}

bool UserModel::offlineAll()
{
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        return mysql.ExecuteNonQuery(sql);
    }
    return false;
}