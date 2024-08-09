#pragma once

#include "base/Timestamp.h"
#include "base/Copyable.h"
#include "http/HttpConnection.h"
#include "http/ByteData.h"
#include "http/parser/HttpParser.h"
#include "http/core/HttpMultipart.h"
#include "http/core/HttpCode.h"
#include "http/core/HttpRequest.h"
#include "http/core/HttpResponse.h"


namespace Miren {
namespace http {

class HttpSession : public std::enable_shared_from_this<HttpSession>{
public:
    typedef std::function<void (std::shared_ptr<HttpSession>)> RequestCallback;
    HttpSession(std::shared_ptr<HttpConnection> conn, RequestCallback cb);

    ~HttpSession();
    std::unique_ptr<HttpRequest>& getRequest() { return http_request_; }
    std::unique_ptr<HttpResponse>& getResponse() { return http_response_; }

    void setRequestCallback(RequestCallback cb) { requestCallback_ = std::move(cb); }
    bool parse(Miren::net::Buffer* buf, Miren::base::Timestamp receivetime);
    void sendString(const HttpStatusCode& code, const std::string& data);
    void sendJson(const HttpStatusCode& code, const std::string& data);
    void sendFile(const HttpStatusCode& code, const std::string& filepath,  const std::string& filename);
    //void SendMedia(HttpStatusCode code, const std::string& filepath, //type)
    //void SendBinary()
    //void SendHtml();
    void sendMultipart(const HttpStatusCode& code, const std::vector<MultipartPart*>& parts);
    void send(ByteData* data);


public:
    std::shared_ptr<HttpConnection> connection_;
    RequestCallback requestCallback_;
private:
    std::unique_ptr<HttpRequest> http_request_;
    std::unique_ptr<HttpResponse> http_response_;
    bool need_close_ = true;
    bool handleMessage(Miren::net::Buffer* buf, Miren::base::Timestamp receivetime);
public:
    void handleParsedMessage();
    std::string generateBoundary(size_t len);
};

}
}

