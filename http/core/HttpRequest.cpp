#include "http/core/HttpRequest.h"
#include "base/StringUtil.h"
namespace Miren
{
namespace http
{

#define PARSE_PARAM(str, m, flag, trim) \
    size_t pos = 0; \
    do { \
        size_t last = pos; \
        pos = str.find('=', pos); \
        if(pos == std::string::npos) { \
            break; \
        } \
        size_t key = pos; \
        pos = str.find(flag, pos); \
        m.insert(std::make_pair(trim(str.substr(last, key - last)), \
                    Miren::base::StringUtil::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
        if(pos == std::string::npos) { \
            break; \
        } \
        ++pos; \
    } while(true);


HttpRequest::HttpRequest(HttpVersion version, bool close)
    :method_(llhttp_method::HTTP_GET)
    ,version_(version)
    ,close_(close)
    ,url_("/")
    ,init_(false) {
}

std::string HttpRequest::getHeader(const std::string& key
                            ,const std::string& def) const {
    auto it = headers_.find(key);
    return it == headers_.end() ? def : it->second;
}

void HttpRequest::setHeader(const std::string& key, const std::string& val) {
    headers_[key] = val;
}

void HttpRequest::delHeader(const std::string& key) {
    headers_.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string* val) {
    auto it = headers_.find(key);
    if(it == headers_.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

std::string HttpRequest::getParam(const std::string& key
                            ,const std::string& def) {
    auto it = params_.find(key);
    return it == params_.end() ? def : it->second;
}

void HttpRequest::setParam(const std::string& key, const std::string& val) {
    params_[key] = val;
}

void HttpRequest::delParam(const std::string& key) {
    params_.erase(key);
}

bool HttpRequest::hasParam(const std::string& key, std::string* val) {
    auto it = params_.find(key);
    if(it == params_.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}

std::string HttpRequest::getCookie(const std::string& key
                            ,const std::string& def) {
    initCookies();
    auto it = cookies_.find(key);
    return it == cookies_.end() ? def : it->second;
}

void HttpRequest::setCookie(const std::string& key, const std::string& val) {
    cookies_[key] = val;
}

void HttpRequest::delCookie(const std::string& key) {
    cookies_.erase(key);
}

bool HttpRequest::hasCookie(const std::string& key, std::string* val) {
    initCookies();
    auto it = cookies_.find(key);
    if(it == cookies_.end()) {
        return false;
    }
    if(val) {
        *val = it->second;
    }
    return true;
}


std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << methodString() << " "
       << url_
       << " "
       << HttpVersionToString(version_)
       << "\r\n";
    for(auto& i : headers_) {
        os << i.first << ": " << i.second << "\r\n";
    }
    os << "\r\n";
    if(!body_.empty()) {
        os << body_;
    }
    return os;
}

bool HttpRequest::init() {
    if(init_) {
        return true;
    }
    std::string conn = getHeader("connection");
    if(!conn.empty()) {
        if(strcasecmp(conn.c_str(), "keep-alive") == 0) {
            close_ = false;
        } else {
            close_ = true;
        }
    }
    initParam();
    bool ret = initBody();
    init_ = true;
    return ret;
}

void HttpRequest::initParam() {
    initQueryParam();
    initCookies();
}


void HttpRequest::initQueryParam() {
    PARSE_PARAM(requestUrl_.query, params_, '&',);
}

bool HttpRequest::initBody() {
    if(body_.empty()) {
        return true;
    }
    std::string content_type = getHeader("content-type");
    if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr) {
        multiBody_.reset(HttpRequestBody::HttpRequestBodyFactory(content_type));
        if(multiBody_ != nullptr) {
            return multiBody_->setData(body_.c_str(), body_.size());
        }
        return true;
    }
    PARSE_PARAM(body_, params_, '&',);
    return true;
}

void HttpRequest::initCookies() {
    std::string cookie = getHeader("cookie");
    if(cookie.empty()) {
        return;
    }
    PARSE_PARAM(cookie, cookies_, ';', base::StringUtil::Trim);
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

// HTTP request
void HttpRequest::reset() {
  method_ = llhttp_method::HTTP_GET;
  url_.clear();
  headers_.clear();
  body_.clear();
}

// void HttpRequest::Swap(HttpRequest& other) {
//   std::swap(method_, other.method_);
//   url_.swap(other.url_);
//   headers_.swap(other.headers_);
//   body_.swap(other.body_);
// }

} // namespace http

} // namespace Miren
