#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include "http/HttpConnection.h"
#include "http/HttpSession.h"

namespace Miren 
{
namespace http
{
class HttpConnection;
class HttpContext : public std::enable_shared_from_this<HttpContext> {
private:
    typedef std::function<void(std::shared_ptr<HttpContext>)> HttpContextHandler;
    int index_ = -1;
    std::vector<HttpContextHandler> handlers_;  //处理器链
    //shared 避免下层通道关闭后 上层无感知导致send空指针
    std::shared_ptr<HttpSession> session_;
    std::unordered_map<std::string, std::string> router_params_;       //动态路由的参数
    // std::shared_ptr<Redis> redis_ = nullptr;
    // std::shared_ptr<MySQL> mysql_ = nullptr;
    
public:
    friend class HttpRouter;
    HttpContext(std::shared_ptr<HttpSession> session) : session_(session){}
    ~HttpContext();
    
    void Next() {
        ++index_;
        if(index_ < handlers_.size()) {
            (handlers_[index_])(shared_from_this());
        }
    }
    
    void AddHandler(HttpContextHandler handler) {
        handlers_.push_back(std::move(handler));
    }

    std::string Method() const;
    const std::string& Path() const;
    std::string RouterParam(const std::string& key, const std::string& def = "");
    std::string Query(const std::string& key, const std::string& def = "") const;
    std::string PostForm(const std::string& key, const std::string& def = "") const;
    std::shared_ptr<MultipartPart> MultipartForm(const std::string& key, size_t idx = 0) const;
    const Json::Value JsonValue() const;

    // std::shared_ptr<Redis> Redis();
    // std::shared_ptr<MySQL> MySQL();

    void SaveUploadedFile(const BinaryData& file, const std::string& path, const std::string& filename);
    
    void STRING(const llhttp_status& code, const std::string& data);
    void JSON(const llhttp_status& code, const std::string& data);
    void FILE(const llhttp_status& code, const std::string& filepath, std::string filename = "");
    void MULTIPART(const llhttp_status& code, const std::vector<MultipartPart*>& parts);
};



} // namespace http
}

