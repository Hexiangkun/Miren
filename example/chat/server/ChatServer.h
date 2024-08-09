#pragma once

#include "net/EventLoop.h"
#include "net/sockets/InetAddress.h"
#include "net/TcpConnection.h"
#include "net/TcpServer.h"

class ChatServer {
public:
    ChatServer(Miren::net::EventLoop* loop, const Miren::net::InetAddress& listenAddr, const std::string& nameArg);

    ~ChatServer() = default;

    void start();

    void setThreadNum(int num);

private:
    void onConnection(const Miren::net::TcpConnectionPtr& connection);
    void onMessage(const Miren::net::TcpConnectionPtr& connection, Miren::net::Buffer* buffer, Miren::base::Timestamp stamp);

    Miren::net::TcpServer _server;
};
