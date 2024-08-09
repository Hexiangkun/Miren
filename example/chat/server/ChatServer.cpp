//
// Created by 37496 on 2024/3/16.
//

#include "example/chat/server/ChatServer.h"
#include "example/chat/service/ChatService.h"
#include "base/log/Logging.h"

using namespace std::placeholders;
ChatServer::ChatServer(Miren::net::EventLoop *loop, const Miren::net::InetAddress &listenAddr,
                       const std::string &nameArg) : _server(loop, listenAddr, nameArg){
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

}

void ChatServer::setThreadNum(int num) {
    _server.setThreadNum(num);
}

void ChatServer::start() {
    _server.start();
}

void ChatServer::onConnection(const Miren::net::TcpConnectionPtr &connection) {
    if(connection->disconnected()) {
        ChatService::getInstance()->clientCloseException(connection);
        connection->shutdown();
    }
}

void ChatServer::onMessage(const Miren::net::TcpConnectionPtr &connection, Miren::net::Buffer *buffer,
                           Miren::base::Timestamp stamp) {
    std::string msg = buffer->retrieveAllAsString();
    LOG_INFO << connection->name() << " recived " << msg
             << " | at time: " << stamp.toFormattedString();

    nlohmann::json js = nlohmann::json::parse(msg);
    auto handler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    handler(connection, js, stamp);
}