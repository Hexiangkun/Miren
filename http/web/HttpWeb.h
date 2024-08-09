#pragma once

#include <vector>
#include "http/web/HttpRouter.h"
#include "http/HttpSession.h"
#include "http/HttpServer.h"

namespace Miren
{
namespace http {

class HttpGroup {
private:
    const std::string prefix_;
    std::vector<HttpContextHandler> middlewares_;
    HttpRouter* router_ = nullptr;
    
public:
    friend class HttpWeb;
    HttpGroup(const std::string& prefix, HttpRouter* router) : prefix_(prefix), router_(router) {}
    
    void Use(HttpContextHandler handler) {
        middlewares_.push_back(std::move(handler));
    }
    
    void GET(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("GET", prefix_ + path, handler);
    }
    
    void POST(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("POST", prefix_ + path, handler);
    }
    
    void PUT(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("PUT", prefix_ + path, handler);
    }
    
    void DELETE(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("DELETE", prefix_ + path, handler);
    }
};

class HttpWeb {
private:
    std::unique_ptr<HttpRouter> router_;
    std::vector<HttpContextHandler> global_handlers_;
    std::vector<HttpGroup*> groups_;
    std::unique_ptr<HttpServer> httpserver_;
    
    void serverHTTP(std::shared_ptr<HttpSession> session);
    
public:
    HttpWeb(net::EventLoop* loop,
             const net::InetAddress& listenAddr,
             const std::string& name,
             HttpTcpServer::Option option = HttpTcpServer::kNoReusePort);
    
    ~HttpWeb();
    
    void Use(HttpContextHandler handler) {
        global_handlers_.push_back(std::move(handler));
    }
    
    void GET(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("GET", path, handler);
    }
    
    void POST(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("POST", path, handler);
    }
    
    void PUT(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("PUT", path, handler);
    }
    
    void DELETE(const std::string& path, HttpContextHandler handler) {
        router_->addRouter("DELETE", path, handler);
    }
    
    class HttpGroup* Group(const std::string& prefix) {
        class HttpGroup* group = new class HttpGroup(prefix, router_.get());
        groups_.push_back(std::move(group));
        return group;
    }
    
    void Run(int threadcnt);
};

}
}