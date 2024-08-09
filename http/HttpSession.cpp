#include "http/HttpSession.h"

#include <fcntl.h>
#include <random>
#include <sys/stat.h>
#include <algorithm>
#include "base/log/Logging.h"

namespace Miren {
namespace http {

HttpSession::HttpSession(std::shared_ptr<HttpConnection> conn, RequestCallback cb)
            : connection_(conn), requestCallback_(std::move(cb))
{

}

HttpSession::~HttpSession() {
    http_request_.reset();
    http_response_.reset();
}

bool HttpSession::parse(Miren::net::Buffer* buf, Miren::base::Timestamp receivetime)
{
    return handleMessage(buf, receivetime);
}

bool HttpSession::handleMessage(Miren::net::Buffer* buf, Miren::base::Timestamp receivetime) 
{
    HttpRequestParser http_request_parser_;
    auto req_str = buf->retrieveAllAsString();
    size_t s = http_request_parser_.execute(req_str.data(), req_str.size());
    LOG_INFO << s;
    if(http_request_parser_.isFinished() != 1 || s != req_str.size()) {
        return false;
    }
    else {
        http_request_ = std::move(http_request_parser_.getData());
        LOG_INFO <<( http_request_parser_.getData() == nullptr);
        http_request_->init();
        return true;
    }
}

void HttpSession::handleParsedMessage() {
    auto headers = http_request_->getHeaders();
    
    auto iter = headers.find("connection");
    HttpVersion version = http_request_->getVersion();
    if(version == HttpVersion::HTTP_1_0) {
        if(iter != headers.end() && iter->second == "Keep-Alive") {
            //长连
            need_close_ = false;
        }
    }else if(version == HttpVersion::HTTP_1_1) {
        if(iter == headers.end() || iter->second == "close") {
            need_close_ = false;
        }
    }
    if(requestCallback_) {
        requestCallback_(shared_from_this());
    }

}

void HttpSession::sendString(const HttpStatusCode& code, const std::string& data) {
    http_response_.reset(new HttpResponse(http_request_->getVersion(), http_request_->isClose()));
    http_response_->setStatus(code);

    http_response_->setHeader("Content-Type", HttpContentType2Str.at(HttpContentType::TXT));
    // http_response_->setBody(data);
    http_response_->setHeader("content-length", std::to_string(data.size()));
    std::string header = http_response_->headerToString();
    ByteData* bdata = new ByteData();
    bdata->addDataZeroCopy(header);
    bdata->addDataZeroCopy(data);

    send(bdata);
    LOG_INFO << bdata->remain();
}

void HttpSession::sendJson(const HttpStatusCode& code, const std::string& data) {
    http_response_.reset(new HttpResponse(http_request_->getVersion(), http_request_->isClose()));

    http_response_->setStatus(code);
    http_response_->setHeader("Content-Type", HttpContentType2Str.at(HttpContentType::JSON));
    http_response_->setBody(data);
 
    ByteData* bdata = new ByteData();
    std::string tmp = http_response_->toString();
    bdata->addDataZeroCopy(tmp);
    send(bdata);
}

void HttpSession::sendFile(const HttpStatusCode& code, const std::string& filepath, const std::string& file_name) {
    std::string filename = file_name;
    if(file_name.size() == 0) {
        size_t pos = filepath.find_last_of('/');
        if(pos != std::string::npos) {
            filename = filepath.substr(pos + 1);
        }
    }

    std::string cotent_filetype = filepath.substr(filepath.find_last_of("."));
    std::transform(cotent_filetype.begin(), cotent_filetype.end(), cotent_filetype.begin(), ::tolower);

    auto iter = Ext2HttpContentTypeStr.find(cotent_filetype);
    if(iter == Ext2HttpContentTypeStr.end()) {
        cotent_filetype = "application/octet-stream";
    }else {
        cotent_filetype = iter->second;
    }

    http_response_.reset(new HttpResponse(http_request_->getVersion(), http_request_->isClose()));

    http_response_->setStatus(code);
    http_response_->setHeader("Content-Type", cotent_filetype);
    http_response_->setHeader("Content-Disposition", "attachment; filename=" + filename);

    int fd = open(filepath.c_str(), O_RDONLY);
    assert(fd > 0);
    struct stat st;
    fstat(fd, &st);
    http_response_->setHeader("Content-Length", std::to_string(st.st_size));
    std::string header = http_response_->headerToString();
    ByteData* bdata = new ByteData();
    bdata->addDataZeroCopy(header);
    bdata->addFile(fd, st.st_size);
    send(bdata);
}

void HttpSession::sendMultipart(const HttpStatusCode& code, const std::vector<MultipartPart*>& parts) 
{
    http_response_.reset(new HttpResponse(http_request_->getVersion(), http_request_->isClose()));

    std::string boundary = generateBoundary(16);
    http_response_->setStatus(code);
    http_response_->setHeader("Content-Type", HttpContentType2Str.at(HttpContentType::MULTIPART) + "; boundary=" + boundary);
    
    std::string begin_boundary = "\r\n--" + boundary + "\r\n";
    std::string end_boundary = "\r\n--" + boundary + "--";

    int totalsize = 0;
    for(MultipartPart* part : parts) {
        totalsize += begin_boundary.size() + part->headerToString().size() + part->size();
    }
    totalsize += end_boundary.size() - 2;
    http_response_->setHeader("Content-Length", std::to_string(totalsize));
    
    
    std::string header = http_response_->headerToString();
    LOG_INFO << header;
    header.pop_back(); header.pop_back();       // 删除"\r\n"
    ByteData* bdata = new ByteData();
    bdata->addDataZeroCopy(header);
    
    for(MultipartPart* part : parts) {
        bdata->addDataZeroCopy(begin_boundary);
        bdata->addDataZeroCopy(part->headerToString());
        if(part->fd() > 0) {
            bdata->addFile(part->fd(), part->size());
        }else {
            bdata->addDataZeroCopy(part->data().c_str(), part->size());
        }
    }
    bdata->addDataZeroCopy(end_boundary);
    send(bdata);
}

void HttpSession::send(ByteData* data) {
    connection_->send(data);
}

std::string HttpSession::generateBoundary(size_t len) {
    static std::string charset = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static int length = (int)charset.size();
    std::string boundary = "----";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, length - 1);
    for(int i = 0; i < len; ++i) {
        boundary += charset[dis(gen)];
    }
    return boundary;
}

}
}
