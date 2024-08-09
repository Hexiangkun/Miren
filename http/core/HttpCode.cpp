
#include "http/core/HttpCode.h"
namespace Miren {
namespace http {
std::string HttpVersionToString(HttpVersion ver)   
{
    switch (ver)
    {
    case HTTP_1_0:
        return "HTTP/1.0";
    case HTTP_1_1:
        return "HTTP/1.1";
    case HTTP_2_0:
        return "HTTP/2.0";
    default:
        return "HTTP/1.0";
    }
}

HttpMethod StringToHttpMethod(const std::string& m) {
#define XX(num, name, string) \
    if(strcmp(#string, m.c_str()) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char* m) {
#define XX(num, name, string) \
    if(strncmp(#string, m, strlen(#string)) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const std::string s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const std::string HttpMethodToString(const HttpMethod& m) {
    uint32_t idx = (uint32_t)m;
    if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        return "<unknown>";
    }
    return s_method_string[idx];
}

const std::string HttpStatusToString(const HttpStatusCode& s) {
    switch(s) {
#define XX(code, name, msg) \
        case HttpStatusCode::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

bool CaseInsensitiveLess::operator()(const std::string& lhs
                            ,const std::string& rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


}
}
