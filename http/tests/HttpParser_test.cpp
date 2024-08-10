#include "http/parser/HttpParser.h"
#include <iostream>

const char test_request_data[] = "GET https://translate.google.cn/?sl=zh-CN&tl=en&text=%E9%92%A6%E4%BD%A9&op=translate HTTP/1.0\r\n"
                                "Host: www.sylar.top\r\n"
                                "Cookie: JSESSIONID=0000AgK4N-vgetNoKBOfYd_hJQP:-1; ECSNSessionID=721303315959898497; ASPSESSIONIDQQSCRBSQ=OMFFMGDCJHLLHCLPGMKCEOEG; ASPSESSIONIDCCCRTRDD=KMENDGIBFBKFDLHKKPJGJNMF\r\n"
                                "\r\n";

const char* data = "POST / HTTP/1.1\r\n"
                    "Host: www.example.com\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: 4\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "q=4";

void test_request() {
    Miren::http::HttpParser parser;
    std::string tmp = data;
    size_t offset = 0;
    size_t s = parser.execute(&tmp[0], tmp.size(), &offset);
    std::cout << "execute rt=" << s << std::endl
        << " has_error=" << parser.errorReason() << std::endl
        << " is_finished=" << parser.isComplete() << std::endl
        << " is paused=" << parser.isPause() << std::endl
        << " total=" << tmp.size() << std::endl
        << " offset=" << offset << std::endl;
    tmp += "0";
    Miren::http::HttpParser parser1;
    offset = 0;
        s = parser1.execute(&tmp[0], tmp.size(), &offset);
        std::cout << "execute rt=" << s << std::endl
        << " has_error=" << parser1.errorReason() << std::endl
        << " is_finished=" << parser1.isComplete() << std::endl
        << " is paused=" << parser1.isPause() << std::endl
        << " total=" << tmp.size() << std::endl
        << " offset=" << offset << std::endl;
    std::cout << parser1.request()->toString();
    std::cout << std::endl << "------------" << std::endl;
    auto& req = parser1.request();
    std::cout << req->getHeader("cookie") << std::endl;
    std::cout << req->getHeader("content-length") << std::endl;
    std::cout << req->getParam("sl") << std::endl;
    std::cout << req->getParam("text") << std::endl;

}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response() {
    Miren::http::HttpParser parser(llhttp_type::HTTP_RESPONSE);
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    std::cout << "execute rt=" << s
        << " has_error=" << parser.errorReason()
        << " is_finished=" << parser.isComplete()
        << " total=" << tmp.size();

    std::cout << parser.response()->toString() << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    std::cout << "--------------------------------------------------------" << std::endl;
    // test_response();
    return 0;
}
