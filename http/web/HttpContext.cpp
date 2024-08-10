#include "http/web/HttpContext.h"
#include <random>
#include <fstream>

namespace Miren
{
namespace http
{

HttpContext::~HttpContext() {
    // if(redis_) {
    //     RedisPoolSingleton::GetInstance()->ReleaseConnection(redis_);
    // }
    
    // if(mysql_) {
    //     MySQLPoolSingleton::GetInstance()->ReleaseConnection(mysql_);
    // }
}

std::string HttpContext::Method() const {
  return session_->getRequest()->methodString();
}

const std::string& HttpContext::Path() const {
  return session_->getRequest()->getRequestUrl().path;
}

std::string HttpContext::RouterParam(const std::string& key, const std::string& def) {
    auto iter = router_params_.find(key);
    if(iter != router_params_.end()) {
        return iter->second;
    }
    return def;
}

std::string HttpContext::Query(const std::string &key, const std::string& def) const {
    return session_->getRequest()->getParam(key, def);
}

std::string HttpContext::PostForm(const std::string &key, const std::string& def) const {
    return session_->getRequest()->getParam(key, def);
}

std::shared_ptr<MultipartPart> HttpContext::MultipartForm(const std::string& key, size_t idx) const {
    try{
        return std::dynamic_pointer_cast<HttpRequestMultipartBody>(session_->getRequest()->mutlipartBody())->getFormFile(key, idx);
    }
    catch(std::exception&) {

    }
    return nullptr;
}

const Json::Value HttpContext::JsonValue() const {
    try{
        return std::dynamic_pointer_cast<HttpRequestJsonBody>(session_->getRequest()->mutlipartBody())->jsonValue();
    }
    catch(std::exception&) {

    }
    return Json::Value();
}

void HttpContext::SaveUploadedFile(const BinaryData &file, const std::string &path, const std::string& filename) {
    std::ofstream ofs;
    ofs.open(path + filename, std::ofstream::out | std::ofstream::app);
    ofs.write(file.data, file.size);
    ofs.close();
}

// std::shared_ptr<Redis> HttpContext::Redis() {
//     if(redis_) return redis_;
//     redis_ = RedisPoolSingleton::GetInstance()->GetConnection();
//     return redis_;
// }

// std::shared_ptr<MySQL> HttpContext::MySQL() {
//     if(mysql_) return mysql_;
//     mysql_ = MySQLPoolSingleton::GetInstance()->GetConnection();
//     return mysql_;
// }


void HttpContext::STRING(const llhttp_status& code, const std::string& data) {
    session_->sendString(code, data);
}

void HttpContext::JSON(const llhttp_status& code, const std::string& data) {
    session_->sendJson(code, data);
}

//单文件传输
void HttpContext::FILE(const llhttp_status& code, const std::string &filepath, std::string filename) {
    session_->sendFile(code, filepath, filename);
}

//MULTIPART 数据
/*
POST /upload HTTP/1.1
Host: example.com
Content-Type: multipart/form-data; boundary=----BOUNDARY_STRING

------BOUNDARY_STRING
Content-Disposition: form-data; name="file"; filename="test.jpg"
Content-Type: image/jpeg

<JPEG file data>
------BOUNDARY_STRING--*/
void HttpContext::MULTIPART(const llhttp_status& code, const std::vector<MultipartPart *>& parts) {
    session_->sendMultipart(code, parts);
}

  
} // namespace http


}
