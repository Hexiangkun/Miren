#include "third_party/llhttp/include/llhttp.h"
#include "http/core/HttpRequest.h"
#include "http/core/HttpResponse.h"
#include <memory>

namespace Miren
{
namespace http
{
//----- HTTP parser ---------
class HttpParser {
 public:
  explicit HttpParser(llhttp_type type= llhttp_type::HTTP_REQUEST);

  HttpParser(const HttpParser&) = delete;
  void operator=(const HttpParser&) = delete;

  bool execute(const std::string& data, size_t* offset = nullptr);
  bool execute(const char* data, size_t len, size_t* offset = nullptr);

//   void SetRequestHandler(HttpRequestHandler h) { req_handler_ = std::move(h); }
//   void SetResponseHandler(HttpResponseHandler h) { rsp_handler_ = std::move(h); }

  bool isRequest() const { return type_ == HTTP_REQUEST; }
  bool isComplete() const { return complete_; }
  bool isPause() const { return pause_; }

  // assert IsRequest() && IsComplete()
  std::unique_ptr<HttpRequest>& request() { return request_; }

  // assert !IsRequest() && IsComplete()
  std::unique_ptr<HttpResponse>& response() { return response_; }

  const std::string& errorReason() const { return error_reason_; }

  // for unit test
  void reinit();
 private:
  void reset();
  void setLength(const char* data) { length_ = data; }
  static int OnMessageBegin(llhttp_t* h);
  static int OnMessageComplete(llhttp_t* h);

  static int OnUrl(llhttp_t* h, const char* data, size_t len);
  static int OnVersion(llhttp_t* h, const char* data, size_t len);
  static int OnHeaderField(llhttp_t* h, const char* data, size_t len);
  static int OnHeaderValue(llhttp_t* h, const char* data, size_t len);
  static int OnHeadersComplete(llhttp_t* h);
  static int OnStatus(llhttp_t* h, const char* data, size_t len);
  static int OnBody(llhttp_t* h, const char* data, size_t len);

  static int OnUrlComplete(llhttp_t* h);
  static int OnStatusComplete(llhttp_t* h);
  static int OnHeaderFieldComplete(llhttp_t* h);
  static int OnHeaderValueComplete(llhttp_t* h);

  llhttp_t parser_;
  llhttp_settings_t settings_;
  bool complete_ = false;
  bool pause_ = false;
  const char* length_ = nullptr;
  const llhttp_type type_;  // request or response
  std::unique_ptr<HttpRequest> request_;
  std::unique_ptr<HttpResponse> response_;

  std::string key_, value_;  // temp vars for parse header
  std::string error_reason_;

//   HttpRequestHandler req_handler_;
//   HttpResponseHandler rsp_handler_;
};


struct Url ParseUrl(const std::string& url);
}
}