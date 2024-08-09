#pragma once

#include "base/Copyable.h"
#include <string>
#include <map>

namespace Miren
{
namespace net
{

class Buffer;

//muduo封装了HttpResponse类将响应实体保存到Buffer对象中，最后通过TcpConnection发送给客户端。
class HttpResponse : public Miren::base::Copyable
{
 public:
  enum HttpStatusCode   //HTTP状态码
  {
    kUnknown,
    k200Ok = 200,   //请求成功
    k301MovedPermanently = 301, //资源被转移，请求将被重定向
    k400BadRequest = 400, //通用客户请求错误(请求没有进入到后台服务里)
    k404NotFound = 404, //资源未找到
  };

  explicit HttpResponse(bool close)
    : statusCode_(kUnknown),
      closeConnection_(close)
  {
  }

  void setStatusCode(HttpStatusCode code)   //设置状态码
  { statusCode_ = code; }

  void setStatusMessage(const std::string& message)  //设置状态码对应的文本信息
  { statusMessage_ = message; }

  void setCloseConnection(bool on)  
  { closeConnection_ = on; }

  bool closeConnection() const
  { return closeConnection_; }

  //Content-Type 字段作用：用来获知请求中的消息主体是用何种方式编码，再对主体进行解析
  void setContentType(const std::string& contentType)
  { addHeader("Content-Type", contentType); }

  // FIXME: replace std::string with StringPiece
  void addHeader(const std::string& key, const std::string& value)
  { headers_[key] = value; }

  void setBody(const std::string& body)
  { body_ = body; }

  void appendToBuffer(Buffer* output) const;  // 将HttpResponse添加到Buffer

 private:
  std::map<std::string, std::string> headers_;  //头列表,字段名-值
  HttpStatusCode statusCode_; //状态码
  // FIXME: add http version
  std::string statusMessage_;  //状态响应码对应的文本信息
  bool closeConnection_;  //是否 keep-alive
  std::string body_; //实体(响应报文)
};

}
}

/*
2、http response：


    status line + header + body （header分为普通报头，响应报头与实体报头）

    header与body之间有一空行（CRLF）

    状态响应码

        1XX  提示信息 - 表示请求已被成功接收，继续处理

        2XX  成功 - 表示请求已被成功接收，理解，接受

        3XX  重定向 - 要完成请求必须进行更进一步的处理

        4XX  客户端错误 -  请求有语法错误或请求无法实现

        5XX  服务器端错误 -   服务器执行一个有效请求失败


一个典型的http 响应：


HTTP/1.1 200 OK

Content-Length: 112

Connection: Keep-Alive

Content-Type: text/html

Server: Muduo
*/
