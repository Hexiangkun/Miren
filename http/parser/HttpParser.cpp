#include "http/parser/HttpParser.h"
#include "http/core/HttpUtil.h"
#include <algorithm>
#include "base/log/Logging.h"

namespace Miren::http {

HttpParser::HttpParser(llhttp_type type) 
  : type_(type),
  request_(nullptr),
  response_(nullptr)
{
  llhttp_settings_init(&settings_);

  // request
  settings_.on_url = &HttpParser::OnUrl;
  settings_.on_version = &HttpParser::OnVersion;
  settings_.on_url_complete = &HttpParser::OnUrlComplete;

  // response
  settings_.on_status = &HttpParser::OnStatus;
  settings_.on_status_complete = &HttpParser::OnStatusComplete;

  // both
  settings_.on_message_begin = &HttpParser::OnMessageBegin;
  settings_.on_message_complete = &HttpParser::OnMessageComplete;
  settings_.on_header_field = &HttpParser::OnHeaderField;
  settings_.on_header_value = &HttpParser::OnHeaderValue;
  settings_.on_headers_complete = &HttpParser::OnHeadersComplete;
  settings_.on_header_field_complete = &HttpParser::OnHeaderFieldComplete;
  settings_.on_header_value_complete = &HttpParser::OnHeaderValueComplete;
  settings_.on_body = &HttpParser::OnBody;

  llhttp_init(&parser_, type, &settings_);
  parser_.data = this;
}

bool HttpParser::execute(const std::string& data, size_t* offset) { return execute(data.c_str(), data.size(), offset); }

bool HttpParser::execute(const char* data, size_t len, size_t* offset) {
  const char* start = data;
  if (!error_reason_.empty()) {
    return false;
  }

  auto err = llhttp_execute(&parser_, data, len);
  *offset = size_t(length_ - start);
  
  if (err != HPE_OK) {
    if(pause_) {
      error_reason_ = "body length is not equal content-length";
      return false;
    }
    if((*offset) == (size_t)(llhttp_get_error_pos(&parser_) - start)) {
      return true;
    }
    error_reason_ = llhttp_get_error_reason(&parser_);
    return false;
  }

  return true;
}

void HttpParser::reset() {
  complete_ = false;
  length_ = nullptr;
  llhttp_reset(&parser_);

  request_ = std::make_unique<HttpRequest>();
  response_ = std::make_unique<HttpResponse>();

  key_.clear();
  value_.clear();
  error_reason_.clear();
}

int HttpParser::OnMessageBegin(llhttp_t* h) {
  HttpParser* parser = (HttpParser*)h->data;
  parser->reset();

  return 0;
};

int HttpParser::OnMessageComplete(llhttp_t* h) {
  HttpParser* parser = (HttpParser*)h->data;

  parser->complete_ = true;
  if (parser->isRequest()) {
    parser->request_->setMethod(llhttp_method_t(parser->parser_.method));
    // if (parser->req_handler_) {
    //   parser->req_handler_(parser->request_);
    // }
  } else {
    parser->response_->setStatusCode((llhttp_status)parser->parser_.status_code);
    // if (parser->rsp_handler_) {
    //   parser->rsp_handler_(parser->response_);
    // }
  }

  return 0;
}

int HttpParser::OnUrl(llhttp_t* h, const char* data, size_t len) {
  HttpParser* parser = (HttpParser*)h->data;
  parser->setLength(data);
  if (!parser->isRequest()) {
    return -1;
  }
  parser->request_->appendUrl(data, len);

  return 0;
}

int HttpParser::OnVersion(llhttp_t* h, const char* data, size_t len) {
  HttpParser* parser = (HttpParser*)h->data;
  parser->setLength(data);

  if (!parser->isRequest()) {
    parser->response_->setVersion(data, len);
  }
  else {
    parser->request_->setVersion(data, len);
  }
  return 0;
}

int HttpParser::OnHeaderField(llhttp_t* h, const char* data, size_t len) {
  if (len > 0) {
    HttpParser* parser = (HttpParser*)h->data;
    parser->setLength(data);

    parser->key_.append(data, len);
  }

  return 0;
}

int HttpParser::OnHeaderValue(llhttp_t* h, const char* data, size_t len) {
  if (len > 0) {
    HttpParser* parser = (HttpParser*)h->data;
    parser->setLength(data);

    parser->value_.append(data, len);
  }

  return 0;
}

int HttpParser::OnStatus(llhttp_t* h, const char* data, size_t len) {
  if (len == 0) {
    return 0;
  }

  HttpParser* parser = (HttpParser*)h->data;
  parser->setLength(data);

  if (parser->isRequest()) {
    return -1;
  }
  parser->response_->appendStatusReason(data, len);

  return 0;
}

int HttpParser::OnHeadersComplete(llhttp_t* h) { return 0; }

int HttpParser::OnBody(llhttp_t* h, const char* data, size_t len) {
  HttpParser* parser = (HttpParser*)h->data;
  parser->setLength(data);

  if(parser->request_->hasHeader("Content-Length")) {
    size_t length = parser->request_->getHeaderAs<size_t>("Content-Length");
    if(len < length) {
      parser->pause_ = true;
      return -1;
    }

    if (parser->isRequest()) {
      parser->request_->appendBody(data, length);
    } else {
      parser->response_->appendBody(data, length);
    }
    parser->setLength(data+length);
    return 0;
  }
  return 0;
}

int HttpParser::OnUrlComplete(llhttp_t* h) { 
  HttpParser* parser = (HttpParser*)h->data;

  auto result = ParseUrl(parser->request_->url());
  parser->request_->setRequstUrl(result);
  return 0; 
}

int HttpParser::OnStatusComplete(llhttp_t* h) { return 0; }

int HttpParser::OnHeaderFieldComplete(llhttp_t* h) { return 0; }

inline std::string& Trim(std::string& s) {
  if (s.empty()) {
    return s;
  }

  s.erase(0, s.find_first_not_of(" "));
  s.erase(s.find_last_not_of(" ") + 1);
  return s;
}

int HttpParser::OnHeaderValueComplete(llhttp_t* h) {
  HttpParser* parser = (HttpParser*)h->data;
  Trim(parser->key_);
  if (parser->key_.empty()) {
    return HPE_INVALID_HEADER_TOKEN;
  }

  if (parser->isRequest()) {
    parser->request_->setHeader(parser->key_, parser->value_);
  } else {
    parser->response_->setHeader(parser->key_, parser->value_);
  }

  parser->key_.clear();
  parser->value_.clear();
  return 0;
}

void HttpParser::reInit() {
  if (error_reason_.empty()) {
    return;
  }

  error_reason_.clear();
  llhttp_init(&parser_, type_, &settings_);
  parser_.data = this;
  reset();
}


struct Url ParseUrl(const std::string& url_) {
  std::string url = urlDecode(url_);
  struct Url result;
  if (url.empty()) {
    return result;
  }

  const char* start = &url[0];
  const char* end = start + url.size();
  const char* query = std::find(start, end, '?');
  if (query != end) {
    result.query = std::string(query + 1, end);
  }

  result.path = std::string(start, query);
  return result;
}

}  // namespace ananas
