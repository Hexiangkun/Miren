#include "http/web/HttpWeb.h"
#include "http/web/HttpContext.h"
#include "http/HttpServer.h"

namespace Miren {
namespace http
{
HttpWeb::HttpWeb(net::EventLoop* loop,
             const net::InetAddress& listenAddr,
             const std::string& name,
             HttpTcpServer::Option option) {
    router_.reset(new HttpRouter());
    httpserver_.reset(new HttpServer(loop, listenAddr, name, option));
    httpserver_->setRequestCallback(std::bind(&HttpWeb::serverHTTP, this, std::placeholders::_1));
}

HttpWeb::~HttpWeb() {
    for(class HttpGroup* group : groups_) {
        delete group;
    }
}

void HttpWeb::serverHTTP(std::shared_ptr<HttpSession> session) {
    std::shared_ptr<HttpContext> c = std::make_shared<HttpContext>(session);
    
    //全局中间件
    for(HttpContextHandler handler : global_handlers_) {
        //验证有无问题
        c->AddHandler(std::move(handler));
    }
    
    for(class HttpGroup* group : groups_) {
        if(c->Path().find(group->prefix_) == 0) {
            for(HttpContextHandler handler : group->middlewares_) {
                c->AddHandler(std::move(handler));
            }
        }
    }
    
    router_->handle(c);
}

void HttpWeb::Run(int threadcnt) {
    httpserver_->setThreadNum(threadcnt);
    httpserver_->start();
}
  
} // namespace http

}