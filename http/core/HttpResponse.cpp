#include "http/core/HttpResponse.h"
#include "base/Util.h"

namespace Miren
{
namespace http
{

HttpResponse::HttpResponse(HttpVersion version, bool close)
    :m_status(HttpStatusCode::OK)
    ,m_version(version)
    ,m_close(close)
    ,m_websocket(false) {
}

std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string& key, const std::string& val) {
    m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string& key) {
    m_headers.erase(key);
}

void HttpResponse::setRedirect(const std::string& uri) {
    m_status = HttpStatusCode::FOUND;
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
    m_cookies.push_back(ss.str());
}


std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::string HttpResponse::headerToString() const {
    std::stringstream ss;
    ss << HttpVersionToString(m_version)
       << " "
       << (uint32_t)m_status
       << " "
       << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
       << "\r\n";
    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        ss << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : m_cookies) {
        ss << "Set-Cookie: " << i << "\r\n";
    }
    if(!m_websocket) {
        ss << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if(!m_body.empty()) {
        ss << "content-length: " << m_body.size() << "\r\n\r\n";
    } else {
        ss << "\r\n";
    }
    return ss.str();
}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    os << HttpVersionToString(m_version)
       << " "
       << (uint32_t)m_status
       << " "
       << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
       << "\r\n";

    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : m_cookies) {
        os << "Set-Cookie: " << i << "\r\n";
    }
    if(!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) {
    return rsp.dump(os);
}

} // namespace http

} // namespace Miren
