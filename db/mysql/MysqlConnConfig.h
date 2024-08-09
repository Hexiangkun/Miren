#pragma once

#include <string>

namespace SqlConn
{
struct MysqlConnConfig
{
std::string ip;
unsigned short port;
std::string dbname;
std::string username;
std::string password;
MysqlConnConfig() : ip("127.0.0.1"), port(3306) {}
MysqlConnConfig(const std::string& ip_, unsigned short port_,
                const std::string& dbname_, const std::string& username_, const std::string& password_)
                :ip(ip_), port(port_), dbname(dbname_), username(username_), password(password_) {}
};
} // namespace SqlConn
