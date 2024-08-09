#include "http/core/HttpMultipart.h"

namespace Miren {
namespace http {

static std::string header_field_temp = "";
HttpRequestBody* HttpRequestBody::HttpRequestBodyFactory(const std::string &content_type) {
    if(content_type == "application/json") {
        return new HttpRequestJsonBody();
    }else if(content_type == "application/x-www-form-urlencoded") {
        return new HttpRequestFormBody();
    }else if (content_type.find("multipart/form-data") == 0) {
        return new HttpRequestMultipartBody(content_type);
    }else {
        return nullptr;
    }
}

bool HttpRequestFormBody::setData(const char* data, size_t size)
{
    const char* start = data;
    const char* end = data + size;
    const char* flag = nullptr;
    const char* equal = nullptr;
    do {
        flag = std::find(start, end, '&');
        equal = std::find(start, flag, '=');
        if(flag == end && equal == flag) {
            return false;
        }
        formdatas_.emplace(std::string(start, equal), std::string(equal + 1, flag));
        start = flag + 1;
    }while(flag != end);
    
    return true;
}

}
}
