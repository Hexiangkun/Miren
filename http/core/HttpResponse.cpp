#include "http/core/HttpResponse.h"
#include "base/Util.h"

namespace Miren
{
namespace http
{

HttpResponse::HttpResponse(HttpVersion version, bool close)
    :status_code_(llhttp_status::HTTP_STATUS_OK)
    ,version_(version)
    ,close_(close)
    ,websocket_(false) {
}

std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const {
    auto it = headers_.find(key);
    return it == headers_.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string& key, const std::string& val) {
    headers_[key] = val;
}

void HttpResponse::delHeader(const std::string& key) {
    headers_.erase(key);
}

void HttpResponse::setRedirect(const std::string& uri) {
    status_code_ = llhttp_status::HTTP_STATUS_NOT_FOUND;
    setHeader("Location", uri);
}

void HttpResponse::setCookie(const std::string& key, const std::string& val,
                             time_t expired, const std::string& path,
                             const std::string& domain, bool secure) {
    std::stringstream ss;
    ss << key << "=" << val;
    if(expired > 0) {
        ss << ";expires=" << base::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    if(!domain.empty()) {
        ss << ";domain=" << domain;
    }
    if(!path.empty()) {
        ss << ";path=" << path;
    }
    if(secure) {
        ss << ";secure";
    }
    cookies_.push_back(ss.str());
}


std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::string HttpResponse::headerToString() const {
    std::stringstream ss;
    ss << HttpVersionToString(version_)
       << " "
       << (uint32_t)status_code_
       << " "
       << (status_reason_.empty() ? getStatusCodeString() : status_reason_)
       << "\r\n";
    
    for(auto& i : headers_) {
        ss << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : cookies_) {
        ss << "Set-Cookie: " << i << "\r\n";
    }
    ss << "\r\n";
    return ss.str();
}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    os << HttpVersionToString(version_)
       << " "
       << (uint32_t)status_code_
       << " "
       << (status_reason_.empty() ? getStatusCodeString() : status_reason_)
       << "\r\n";

    for(auto& i : headers_) {
        os << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : cookies_) {
        os << "Set-Cookie: " << i << "\r\n";
    }

    if(!body_.empty()) {
        os << body_;
    } else {
        os << "\r\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}


// HTTP response
void HttpResponse::reset() {
  status_code_ = llhttp_status::HTTP_STATUS_OK;
  status_reason_.clear();
  headers_.clear();
  body_.clear();
}

// void HttpResponse::Swap(HttpResponse& other) {
//   std::swap(code_, other.code_);
//   status_code_.swap(other.status_code_);
//   headers_.swap(other.headers_);
//   body_.swap(other.body_);
// }
} // namespace http

} // namespace Miren
