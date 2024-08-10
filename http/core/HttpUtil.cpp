
#include "http/core/HttpUtil.h"
#include "base/Util.h"

#include <cstring>
namespace Miren {
namespace http {
size_t murmurHash2(const std::string& s){
    constexpr unsigned int m = 0x5bd1e995;
    constexpr int r = 24;
    // Initialize the hash to a 'random' value
    size_t len = s.size();
    size_t h = 0xEE6B27EB ^ len;

    // Mix 4 bytes at a time into the hash
    const unsigned char *data = (const unsigned char *)s.data();

    while (len >= 4) {
        unsigned int k = *(unsigned int *)data;
        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array
    switch (len) {
        case 3:
            h ^= data[2] << 16;
            [[fallthrough]];
        case 2:
            h ^= data[1] << 8;
            [[fallthrough]];
        case 1:
            h ^= data[0];
            h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

std::string urlEncode(const char *value, size_t size) {
    std::string escaped;

    for (size_t i = 0; i < size; i++) {
        unsigned char c = (unsigned char)value[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped += c;
        }
            else if(c == ' '){     // 可以把' '转为'+'，也可以选择直接转为十六进制编码
                escaped += '+';
            }
        else{
            // split one byte to two char
            // 0xAB => %AB
            char buf[5]{0};
            sprintf(buf, "%%%c%c", toupper(base::dec2hex(c >> 4)), toupper(base::dec2hex(c & 15)));
            escaped += buf;
        }
    }

    return escaped;
}

std::string urlEncode(std::string_view value) {
    return urlEncode(value.data(), value.size());
}
std::string urlEncode(const char * value) {
    return urlEncode(value, strlen(value));
}

std::string urlDecode(const char *value, size_t size) {
    std::string escaped;
    for (size_t i = 0; i < size; ++i) {
        if (value[i] == '%' && i + 2 < size && isxdigit(value[i + 1]) &&
            isxdigit(value[i + 2])) {
            // merge two char to one byte
            // %AB => 0xAB
            unsigned char byte =
                    ((unsigned char)base::hex2dec(value[i + 1]) << 4) | base::hex2dec(value[i + 2]);
            escaped += byte;
            i += 2;
        }
        else if(value[i] == '%'){
            // 解码失败，返回空
            return "";
        }
        else if(value[i] == '+'){
            escaped += ' ';
        }
        else {
            escaped += value[i];
        }
    }
    return escaped;
}

std::string urlDecode(std::string_view value) {
    return urlDecode(value.data(), value.size());
}
std::string urlDecode(const char * value) {
    return urlDecode(value, strlen(value));
}


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

bool CaseInsensitiveLess::operator()(const std::string& lhs
                            ,const std::string& rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


}
}
