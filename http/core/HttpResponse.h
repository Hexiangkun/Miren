#pragma once

#include "third_party/llhttp/include/llhttp.h"
#include "http/core/HttpUtil.h"
#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
namespace Miren
{
namespace http
{
  
/**
 * @brief HTTP响应结构体
 */
class HttpResponse {
public:
    /// HTTP响应结构智能指针
    typedef std::shared_ptr<HttpResponse> ptr;
    /// MapType
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;
    /**
     * @brief 构造函数
     * @param[in] version 版本
     * @param[in] close 是否自动关闭
     */
    HttpResponse(HttpVersion version = HttpVersion::HTTP_1_0, bool close = true);

    void reset();

    /**
     * @brief 返回响应状态
     * @return 请求状态
     */
    llhttp_status getStatusCode() const { return status_code_; }
    std::string getStatusCodeString() const { return llhttp_status_name(status_code_); }

    /**
     * @brief 设置响应状态
     * @param[in] v 响应状态
     */
    void setStatusCode(llhttp_status v) { status_code_ = v;}

    /**
     * @brief 返回响应版本
     * @return 版本
     */
    HttpVersion getVersion() const { return version_;}
    std::string getVersionStr() const { return HttpVersionToString(version_); }
    void setVersion(const HttpVersion& v) { version_ = v; }
    void setVersion(const std::string& ver) {
        version_ = HTTP_1_0;
        if(ver == "1.1") version_ = HTTP_1_1;
        else if(ver == "1.0") version_ = HTTP_1_0;
        else if(ver == "2.0") version_ = HTTP_2_0;
    }
    void setVersion(const char* v, size_t len) { setVersion(std::string(v, len)); }

    /**
     * @brief 设置响应版本
     * @param[in] v 版本
     */
    // void setVersion(uint8_t v) { version_ = v;}
    void setVersion(HttpVersion ver) { version_ = ver; }
    
    /**
     * @brief 返回响应消息体
     * @return 消息体
     */
    const std::string& getBody() const { return body_;}

    /**
     * @brief 设置响应消息体
     * @param[in] v 消息体
     */
    void setBody(const std::string& v) { body_ = v;}
    void appendBody(const std::string& body) { body_ += body; }
    void appendBody(const char* body, size_t len) { body_.append(body, len); }


    /**
     * @brief 返回响应原因
     */
    const std::string& getReason() const { return status_reason_;}

    /**
     * @brief 设置响应原因
     * @param[in] v 原因
     */
    void setStatusReason(const std::string& v) { status_reason_ = v;}
    void appendStatusReason(const std::string& reason) { status_reason_ += reason; }
    void appendStatusReason(const char* reason, size_t len) { status_reason_.append(reason, len); }

    /**
     * @brief 返回响应头部MAP
     * @return MAP
     */
    const MapType& getHeaders() const { return headers_;}

    /**
     * @brief 设置响应头部MAP
     * @param[in] v MAP
     */
    void setHeaders(const MapType& v) { headers_ = v;}

    /**
     * @brief 是否自动关闭
     */
    bool isClose() const { return close_;}

    /**
     * @brief 设置是否自动关闭
     */
    void setClose(bool v) { close_ = v;}

    /**
     * @brief 是否websocket
     */
    bool isWebsocket() const { return websocket_;}

    /**
     * @brief 设置是否websocket
     */
    void setWebsocket(bool v) { websocket_ = v;}

    /**
     * @brief 获取响应头部参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在返回对应值,否则返回def
     */
    std::string getHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 设置响应头部参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void setHeader(const std::string& key, const std::string& val);

    /**
     * @brief 删除响应头部参数
     * @param[in] key 关键字
     */
    void delHeader(const std::string& key);

    /**
     * @brief 检查并获取响应头部参数
     * @tparam T 值类型
     * @param[in] key 关键字
     * @param[out] val 值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(headers_, key, val, def);
    }

    /**
     * @brief 获取响应的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T()) {
        return getAs(headers_, key, def);
    }

    /**
     * @brief 序列化输出到流
     * @param[in, out] os 输出流
     * @return 输出流
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 转成字符串
     */
    std::string toString() const;
    std::string headerToString() const;

    void setRedirect(const std::string& uri);
    void setCookie(const std::string& key, const std::string& val,
                   time_t expired = 0, const std::string& path = "",
                   const std::string& domain = "", bool secure = false);
private:
    /// 响应状态
    llhttp_status status_code_;
    /// 版本
    HttpVersion version_;
    /// 是否自动关闭
    bool close_;
    /// 是否为websocket
    bool websocket_;
    /// 响应消息体
    std::string body_;
    /// 响应原因
    std::string status_reason_;
    /// 响应头部MAP
    MapType headers_;

    std::vector<std::string> cookies_;
};

/**
 * @brief 流式输出HttpResponse
 * @param[in, out] os 输出流
 * @param[in] rsp HTTP响应
 * @return 输出流
 */
std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);


} // namespace http
  
} // namespace Miren
