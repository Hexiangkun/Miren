#pragma once

#include "http/core/HttpUtil.h"
#include "http/core/HttpMultipart.h"
#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include "third_party/llhttp/include/llhttp.h"

namespace Miren
{
namespace http
{
struct Url {
    /// 请求路径
    std::string path;
    /// 请求参数
    std::string query;
    /// @brief 
    std::string fragment;
};
/////
 // @brief HTTP请求结构
 ///
class HttpRequest {
public:
    /// HTTP请求的智能指针
    typedef std::shared_ptr<HttpRequest> ptr;
    /// MAP结构
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    // @brief 构造函数
    // @param[in] version 版本
    // @param[in] close 是否keepalive
    HttpRequest(HttpVersion version = HttpVersion::HTTP_1_0, bool close = true);

    void reset();
    
    // @brief 返回HTTP方法
    llhttp_method method() const { return method_;} 
    void setMethod(llhttp_method meth) { method_ = meth; }
    std::string methodString() const { return llhttp_method_name(method_); } 
    
    
    // @brief 返回HTTP请求的路径
    void setUrl(const std::string& url) { url_ = url; }
    const std::string& url() const { return url_; }
    void appendUrl(const std::string& url) { url_ += url; }
    void appendUrl(const char* url, size_t len) { url_.append(url, len); }


    // @brief 返回HTTP版本
    HttpVersion getVersion() const { return version_;}
    std::string getVersionStr() const { return HttpVersionToString(version_); }
    void setVersion(const HttpVersion& v) { version_ = v; }
    void setVersion(const char* v, size_t len) { setVersion(std::string(v, len)); }
    void setVersion(const std::string& ver) {
        version_ = HTTP_1_0;
        if(ver == "1.1") version_ = HTTP_1_1;
        else if(ver == "1.0") version_ = HTTP_1_0;
        else if(ver == "2.0") version_ = HTTP_2_0;
    }



    // @brief 返回HTTP请求体
    void setBody(const std::string& body) { body_ = body; }
    void setBody(const char* body, size_t len) { body_.assign(body, len); }
    void appendBody(const std::string& body) { body_ += body; }
    void appendBody(const char* body, size_t len) { body_.append(body, len); }
    const std::string& body() const { return body_; }




    // @brief 返回HTTP请求的消息头MAP
    const MapType& getHeaders() const { return headers_;}
    void setHeaders(const MapType& v) { headers_ = v;}
    // @brief 获取HTTP请求的头部参数
    // @param[in] key 关键字
    // @param[in] def 默认值
    // @return 如果存在则返回对应值,否则返回默认值
    std::string getHeader(const std::string& key, const std::string& def = "") const;
    // @brief 设置HTTP请求的头部参数
    // @param[in] key 关键字
    // @param[in] val 值
    void setHeader(const std::string& key, const std::string& val);
     // @brief 删除HTTP请求的头部参数@param[in] key 关键字
    void delHeader(const std::string& key);
    // @brief 判断HTTP请求的头部参数是否存在
    // @param[in] key 关键字
    // @param[out] val 如果存在,val非空则赋值
    // @return 是否存在
    bool hasHeader(const std::string& key, std::string* val = nullptr);
    // @brief 检查并获取HTTP请求的头部参数
    // @tparam T 转换类型
    // @param[in] key 关键字
    // @param[out] val 返回值
    // @param[in] def 默认值
    // @return 如果存在且转换成功返回true,否则失败val=def
    template<class T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(headers_, key, val, def);
    }
    // @brief 获取HTTP请求的头部参数
    // @tparam T 转换类型
    // @param[in] key 关键字
    // @param[in] def 默认值
    // @return 如果存在且转换成功返回对应的值,否则返回def
    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T()) {
        return getAs(headers_, key, def);
    }



    // @brief 返回HTTP请求的参数MAP
    const MapType& getParams() const { return params_;}
    void setParams(const MapType& v) { params_ = v;}
    // @brief 获取HTTP请求的请求参数 @param[in] key 关键字 @param[in] def 默认值 @return 如果存在则返回对应值,否则返回默认值
    std::string getParam(const std::string& key, const std::string& def = "");
    // @brief 设置HTTP请求的请求参数 @param[in] key 关键字 @param[in] val 值
    void setParam(const std::string& key, const std::string& val);
    // @brief 删除HTTP请求的请求参数@param[in] key 关键字
    void delParam(const std::string& key);
    // @brief 判断HTTP请求的请求参数是否存在
    // @param[in] key 关键字
    // @param[out] val 如果存在,val非空则赋值
    // @return 是否存在
    bool hasParam(const std::string& key, std::string* val = nullptr);
    // @brief 检查并获取HTTP请求的请求参数
    // @tparam T 转换类型
    // @param[in] key 关键字
    // @param[out] val 返回值
    // @param[in] def 默认值
    // @return 如果存在且转换成功返回true,否则失败val=def
    template<class T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(params_, key, val, def);
    }
    // @brief 获取HTTP请求的请求参数
    // @tparam T 转换类型
    // @param[in] key 关键字
    // @param[in] def 默认值
    // @return 如果存在且转换成功返回对应的值,否则返回def
    template<class T>
    T getParamAs(const std::string& key, const T& def = T()) {
        return getAs(params_, key, def);
    }

    // @brief 返回HTTP请求的cookie MAP
    const MapType& getCookies() const { return cookies_;}
    void setCookies(const MapType& v) { cookies_ = v;}
    // @brief 获取HTTP请求的Cookie参数@param[in] key 关键字@param[in] def 默认值
    // @return 如果存在则返回对应值,否则返回默认值
    std::string getCookie(const std::string& key, const std::string& def = "");
    // @brief 设置HTTP请求的Cookie参数 @param[in] key 关键字 @param[in] val 值
    void setCookie(const std::string& key, const std::string& val);
    // @brief 删除HTTP请求的Cookie参数 @param[in] key 关键字
    void delCookie(const std::string& key);
    // @brief 判断HTTP请求的Cookie参数是否存在
    // @param[in] key 关键字
    // @param[out] val 如果存在,val非空则赋值
    // @return 是否存在
    bool hasCookie(const std::string& key, std::string* val = nullptr);
    // @brief 检查并获取HTTP请求的Cookie参数
    // @tparam T 转换类型
    // @param[in] key 关键字
    // @param[out] val 返回值
    // @param[in] def 默认值
    // @return 如果存在且转换成功返回true,否则失败val=def
    template<class T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T()) {
        initCookies();
        return checkGetAs(cookies_, key, val, def);
    }
    // @brief 获取HTTP请求的Cookie参数
    // @tparam T 转换类型
    // @param[in] key 关键字
    // @param[in] def 默认值
    // @return 如果存在且转换成功返回对应的值,否则返回def
    template<class T>
    T getCookieAs(const std::string& key, const T& def = T()) {
        initCookies();
        return getAs(cookies_, key, def);
    }


    // @brief 是否自动关闭
    bool isClose() const { return close_;}
    void setClose(bool v) { close_ = v;}


    // @brief 序列化输出到流中
    // @param[in, out] os 输出流
    // @return 输出流
    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    bool init();
    void setRequstUrl(Url& url) { requestUrl_ = url; }
    const Url& getRequestUrl() { return requestUrl_; }
    std::shared_ptr<HttpRequestBody>& mutlipartBody() { return multiBody_; }
private:
    void initParam();
    void initQueryParam();
    bool initBody();
    void initCookies();

private:
    /// HTTP方法
    llhttp_method method_;
    /// HTTP版本
    HttpVersion version_;
    /// 是否自动关闭
    bool close_;

    /// 请求url
    std::string url_;
    Url requestUrl_;

    /// 请求消息体
    std::string body_;
    /// 请求头部MAP
    MapType headers_;
    /// 请求参数MAP
    MapType params_;
    /// 请求Cookie MAP
    MapType cookies_;

    std::shared_ptr<HttpRequestBody> multiBody_;
    bool init_;
};


/////
 // @brief 流式输出HttpRequest
 // @param[in, out] os 输出流
 // @param[in] req HTTP请求
 // @return 输出流
 ///
std::ostream& operator<<(std::ostream& os, const HttpRequest& req);


} // namespace http
} // namespace Miren
