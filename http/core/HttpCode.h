#pragma once

#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>

namespace Miren {
namespace http {
enum HttpVersion {
    HTTP_1_0 = 0x10,
    HTTP_1_1 = 0x11,
    HTTP_2_0 = 0x20,
};

std::string HttpVersionToString(HttpVersion ver) ;

/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

/**
 * @brief HTTP方法枚举
 */
enum class HttpMethod {
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

/**
 * @brief HTTP状态枚举
 */
enum class HttpStatusCode {
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief 将字符串方法名转成HTTP方法枚举
 * @param[in] m HTTP方法
 * @return HTTP方法枚举
 */
HttpMethod StringToHttpMethod(const std::string& m);

/**
 * @brief 将字符串指针转换成HTTP方法枚举
 * @param[in] m 字符串方法枚举
 * @return HTTP方法枚举
 */
HttpMethod CharsToHttpMethod(const char* m);

/**
 * @brief 将HTTP方法枚举转换成字符串
 * @param[in] m HTTP方法枚举
 * @return 字符串
 */
const std::string HttpMethodToString(const HttpMethod& m);

/**
 * @brief 将HTTP状态枚举转换成字符串
 * @param[in] m HTTP状态枚举
 * @return 字符串
 */
const std::string HttpStatusToString(const HttpStatusCode& s);

/**
 * @brief 忽略大小写比较仿函数
 */
struct CaseInsensitiveLess {
    /**
     * @brief 忽略大小写比较字符串
     */
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

/**
 * @brief 获取Map中的key值,并转成对应类型,返回是否成功
 * @param[in] m Map数据结构
 * @param[in] key 关键字
 * @param[out] val 保存转换后的值
 * @param[in] def 默认值
 * @return
 *      @retval true 转换成功, val 为对应的值
 *      @retval false 不存在或者转换失败 val = def
 */
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        val = def;
        return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    } catch (...) {
        val = def;
    }
    return false;
}

/**
 * @brief 获取Map中的key值,并转成对应类型
 * @param[in] m Map数据结构
 * @param[in] key 关键字
 * @param[in] def 默认值
 * @return 如果存在且转换成功返回对应的值,否则返回默认值
 */
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<T>(it->second);
    } catch (...) {
    }
    return def;
}



inline const std::unordered_map<std::string, std::string> Ext2HttpContentTypeStr {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".csv", "text/csv"},
        {".txt", "text/plain"},

        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},

        {".bmp", "image/x-ms-bmp"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".gif", "image/gif"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".webp", "image/webp"},
        {".png", "image/png"},
        {".ico", "image/x-icon"},

        {".wav", "audio/wav"},
        {".weba", "audio/webm"},
        {".mp3", "audio/mpeg"},
        {".oga", "audio/ogg"},

        {".ogv", "video/ogg"},
        {".mpeg", "video/mpeg"},
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"},
        {".wmv", "video/x-ms-wmv"},
        {".webm", "video/webm"},
        {".flv", "video/x-flv"},
        {".m4v", "video/x-m4v"},

        {".7z", "application/x-7z-compressed"},
        {".rar", "application/x-rar-compressed"},
        {".sh", "application/x-sh"},
        {".rss", "application/rss+xml"},
        {".tar", "application/x-tar"},
        {".jar", "application/java-archive"},
        {".gz", "application/gzip"},
        {".zip", "application/zip"},
        {".xhtml", "application/xhtml+xml"},
        {".xml", "application/xml"},
        {".js", "text/javascript"},
        {".pdf", "application/pdf"},
        {".doc", "application/msword"},
        {".xls", "application/vnd.ms-excel"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".eot", "application/vnd.ms-fontobject"},
        {".json", "application/json"},
        {".bin", "application/octet-stream"},
        {".exe", "application/octet-stream"},
        {".dll", "application/octet-stream"},
        {".deb", "application/octet-stream"},
        {".iso", "application/octet-stream"},
        {".img", "application/octet-stream"},
        {".dmg", "application/octet-stream"},
        {".docx","application/nsword"},
        {".xlsx","application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".pptx","application/""vnd.openxmlformats-officedocument.presentationml.presentation"}
};

enum class HttpContentType {
    URLENCODED,
    MULTIPART,
    JSON,
    PLAIN,
    HTML,
    XML,
    XHTML,
    TXT,
    RTF,
    PDF,
    WORD,
    JPG,
    JPEG,
    PNG,
    GIF,
    BMP,
    AVI,
    MP4,
    WEBM,
    CSS,
    JS,
    UNKNOW,
};

inline const std::unordered_map<HttpContentType, std::string> HttpContentType2Str = {
        {HttpContentType::URLENCODED, "application/x-www-form-urlencoded"},
        {HttpContentType::MULTIPART, "multipart/form-data"},
        {HttpContentType::PLAIN, "text/plain"},
        {HttpContentType::HTML, "text/html; charset=utf-8"},
        {HttpContentType::XML, "text/xml"},
        {HttpContentType::XHTML, "application/xhtml+xml"},
        {HttpContentType::TXT, "text/plain"},
        {HttpContentType::RTF, "application/rtf"},
        {HttpContentType::PDF, "application/pdf"},
        {HttpContentType::WORD, "application/nsword"},
        {HttpContentType::JSON, "application/json"},
        {HttpContentType::JPEG, "image/jpeg"},
        {HttpContentType::JPG, "image/jpg"},
        {HttpContentType::PNG, "image/png"},
        {HttpContentType::GIF, "image/gif"},
        {HttpContentType::BMP, "image/bmp"},
        {HttpContentType::AVI, "video/x-msvideo"},
        {HttpContentType::MP4, "video/mp4"},
        {HttpContentType::WEBM, "video/webm"},
        {HttpContentType::CSS, "text/css"},
        {HttpContentType::JS, "text/javascript"},
        {HttpContentType::JSON, "application/json"},
};

inline const std::unordered_map<std::string, HttpContentType> Str2HttpContentType (
        [](){
            std::unordered_map<std::string, HttpContentType> tmp;
            for(auto& [k,v]:HttpContentType2Str) {
                tmp[v]=k;
            }
            return tmp;
        }()
);

inline const std::unordered_map<std::string, HttpContentType> Ext2HttpContentType(
        [](){
            std::unordered_map<std::string, HttpContentType> tmp;
            for(auto& [k, v] : Ext2HttpContentTypeStr) {
                auto it = Str2HttpContentType.find(v);
                if(it != Str2HttpContentType.end()) {
                    tmp[k] = it->second;
                }
            }
            return tmp;
        }()
);

} //namespace http
} //namespace Miren

