#pragma once
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wreturn-local-addr"


#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <memory>
#include <jsoncpp/json/json.h>
#include "base/Util.h"

namespace Miren
{
namespace http
{
class HttpRequestBody {
protected:
    const std::string content_type_ = "";
    
public:
    virtual ~HttpRequestBody() {}
    friend class HttpRequest;
    HttpRequestBody(const std::string& type) : content_type_(type) {}
    virtual bool setData(const char* data, size_t size) = 0;
    static HttpRequestBody* HttpRequestBodyFactory(const std::string& content_type);
};

class HttpRequestFormBody : public HttpRequestBody {
private:
    std::unordered_map<std::string, std::string> formdatas_;
    
public:
    virtual ~HttpRequestFormBody() {}
    HttpRequestFormBody() : HttpRequestBody("application/x-www-form-urlencoded") {}
    virtual bool setData(const char* data, size_t size) override;
    const std::string getFormData(const std::string& key) const
    {
        auto iter = formdatas_.find(key);
        if(iter != formdatas_.end()) {
            return iter->second;
        }
        else {
            return "";
        }
    }
};


class HttpRequestJsonBody : public HttpRequestBody {
private:
    Json::Value json_;
    
public:
    HttpRequestJsonBody() : HttpRequestBody("application/json") {}
    virtual ~HttpRequestJsonBody() {}
    virtual bool setData(const char* data, size_t size) override
    {
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
        return cr->parse(data, data + size, &json_, nullptr);
    }
    const Json::Value& jsonValue() const { return json_; }
};

struct BinaryData {
public:
    const char* data = nullptr;
    size_t size = 0;
    BinaryData(const char* data = nullptr, size_t size = 0) : data(data), size(size) {}
};

struct MultipartPart
{
private:
    std::string name_ = "";          //name
    std::string filename_ = "";      //filename
    std::string content_type_ = "";  //Content-Type
    std::string data_ = "";          // data
    std::string headStr_ = "";       // header

    int fd_ = -1;
    size_t size_ = 0;
    std::shared_ptr<HttpRequestBody> body_ = nullptr;
public:
    MultipartPart() {}
    MultipartPart(std::string name, std::string fn, std::string ft, std::string data)
            : name_(std::move(name)), 
            filename_(std::move(fn)),
            content_type_(std::move(ft)), 
            data_(std::move(data)),
            fd_(-1),
            size_(data_.size()),
            body_(nullptr)
    {}

    int fd() const {return fd_;}
    const size_t& size() const { return size_; }
    const std::string& data() {return data_; }

    const std::string& filename() { return filename_; }
    void setFilename(const std::string& name) { filename_ = name; }
    const std::string& name() { return name_; }
    void setName(const std::string& name) { name_ = name; }
    const std::string& contentType() { return content_type_; }
    void setContentType(const std::string& type) { content_type_ = type; }
    std::shared_ptr<HttpRequestBody>& body() { return body_; }
    
    const std::string& toString() {
        headStr_ = "Content-Disposition: form-data; ";
        if(!name_.empty()) {
            headStr_ += "name=" + name_ ;
            if(!filename_.empty()) {
                headStr_ += "; filename=" + filename_;
            }
            headStr_ += "\r\n";
            if(!content_type_.empty()) {
                headStr_ += "Content-Type: " + content_type_ + "\r\n";
            }
        }
        headStr_ += "\r\n";
        headStr_ += data_;
        return headStr_;
    }

    const std::string& headerToString() {
        headStr_ = "Content-Disposition: form-data; ";
        if(!name_.empty()) {
            headStr_ += "name=" + name_ ;
            if(!filename_.empty()) {
                headStr_ += "; filename=" + filename_;
            }
            headStr_ += "\r\n";
            if(!content_type_.empty()) {
                headStr_ += "Content-Type: " + content_type_ + "\r\n";
            }
        }
        headStr_ += "\r\n";
        return headStr_;
    }

    void setData(const std::string& data) {
        data_ = data;
        size_ = data.size();
    }

    void setFile(const std::string& filepath) {
        fd_ = open(filepath.c_str(), O_RDONLY);
        assert(fd_ > 0);
        struct stat st;
        fstat(fd_, &st);
        size_ = st.st_size;
    }

    void setFile(int fd, size_t size) {
        assert(fd > 0);
        fd_ = fd;
        size_ = size;
    }

    void setImage(const std::string& filepath) {
        data_.clear();
        base::base64_encode_image(filepath, &data_);
        size_ = data_.size();
    }

    const Json::Value& jsonValue() const {
        static Json::Value empty;
        if(content_type_ != "application/json") return empty;
        return std::dynamic_pointer_cast<HttpRequestJsonBody>(body_)->jsonValue();
    }

    const std::string& postForm(const std::string &key) const {
        static std::string empty;
        if(content_type_ != "application/x-www-form-urlencoded") return empty;
        return std::dynamic_pointer_cast<HttpRequestFormBody>(body_)->getFormData(key);
    }

    void setBody() {
        body_.reset(HttpRequestBody::HttpRequestBodyFactory(content_type_));
        if(body_ != nullptr) {
            body_->setData(data_.c_str(), data_.size());
        }
    }
};


class HttpRequestMultipartBody : public HttpRequestBody 
{
public:
    HttpRequestMultipartBody(const std::string& content_type_val = ""): HttpRequestBody("multipart/form-data")
    {
        std::size_t pos = content_type_val.find("boundary=");
        if(pos != std::string::npos) {
            boundary_ = content_type_val.substr(pos + 9);
        }
    }

    virtual bool setData(const char* data, size_t size) override
    {
        if(boundary_.size() == 0) return false;
        return parse(std::string_view(data, size));
    }

    void setBoundary(const std::string &boundary) noexcept { boundary_ = boundary; }
    const std::string &getBoundary() const noexcept { return boundary_; }

    const std::string getFormValue(const std::string &name) const {
        if(form_.count(name)) {
            return form_.at(name);
        }
        return "";
    }

    std::shared_ptr<MultipartPart> getFormFile(const std::string &name, size_t index = 0) const
    {
        std::shared_ptr<MultipartPart> p = nullptr;
        auto it = files_.find(name);
        if(it != files_.end()) {
            if(index >= it->second.size()) {
                return p;
            }
            p = it->second[index];
        }
        return p;
    }
private:
    bool parse(std::string_view body);

private:
    typedef std::map<std::string, std::vector<std::shared_ptr<MultipartPart>>> Files;
    typedef std::map<std::string, std::string> Form;

    enum State {
        start_body,
        start_boundary,
        end_boundary,
        start_content_disposition,
        end_content_disposition,
        start_content_type,
        end_content_type,
        start_content_data,
        end_content_data,
        end_body
    };

    std::string boundary_;
    Form form_;
    mutable Files files_;

    inline constexpr static char CR = '\r';
    inline constexpr static char LF = '\n';
};


} // namespace http
} // namespace Miren
