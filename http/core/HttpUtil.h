#pragma once

#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>

namespace Miren {
namespace http {

size_t murmurHash2(const std::string& s);

/*
字符'a'-'z','A'-'Z','0'-'9','.','-','*'和'_' 都不被编码，维持原值；
空格' '被转换为加号'+'。
其他每个字节都被表示成"%XY"的格式，X和Y分别代表一个十六进制位。编码为UTF-8。
*/
//编码
std::string urlEncode(const char *, size_t);
std::string urlEncode(std::string_view);
std::string urlEncode(const char *);

//解码
std::string urlDecode(const char *, size_t);
std::string urlDecode(std::string_view);
std::string urlDecode(const char *);


enum HttpVersion {
    HTTP_1_0 = 0x10,
    HTTP_1_1 = 0x11,
    HTTP_2_0 = 0x20,
};

std::string HttpVersionToString(HttpVersion ver) ;


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

