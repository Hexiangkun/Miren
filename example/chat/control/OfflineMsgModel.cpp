//
// Created by 37496 on 2024/3/15.
//

#include "example/chat/control/OfflineMsgModel.h"
#include "db/mysql/MysqlConnection.h"

// 存储用户的离线消息
bool OfflineMsgModel::insert(const int usrID, const std::string &msg)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage(userid, message) values(%d, '%s')",usrID, msg.c_str());

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        // 插入数据
        if (mysql.ExecuteNonQuery(sql))
        {
            return true;
        }
    }
    return false;
}
// 删除用户的离线消息
bool OfflineMsgModel::remove(const int usrID)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d",usrID);

    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        // 插入数据
        if (mysql.ExecuteNonQuery(sql))
        {
            return true;
        }
    }
    return false;
}
// 将数据库的离线消息输出到一个队列中去
bool OfflineMsgModel::query(const int usrID, std::vector<std::string> &vec)
{
    // 组装sql语句
    char sql[1024];
    sprintf(sql, "select message from offlinemessage where userid = %d", usrID);

    // 创建MySQL对象操作MySQL数据库
    bool ret = false;
    SqlConn::MySqlConnection mysql;
    if (mysql.connect("127.0.0.1", "root", "root", "chat"))
    {
        SqlConn::MySqlDataReader* rd = mysql.ExecuteReader(sql);
        if (rd != nullptr)
        {
            while(rd->Read()) {
                std::string msg;
                rd->GetValues(msg);
                vec.push_back(msg);
            }
            ret = true;
        }
    }
    return ret;
}