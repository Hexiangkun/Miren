#include "http/parser/HttpParser.h"
#include <iostream>

const char test_request_data[] = "GET https://translate.google.cn/?sl=zh-CN&tl=en&text=%E9%92%A6%E4%BD%A9&op=translate HTTP/1.0\r\n"
                                "Host: www.sylar.top\r\n"
                                "Cookie: JSESSIONID=0000AgK4N-vgetNoKBOfYd_hJQP:-1; ECSNSessionID=721303315959898497; ASPSESSIONIDQQSCRBSQ=OMFFMGDCJHLLHCLPGMKCEOEG; ASPSESSIONIDCCCRTRDD=KMENDGIBFBKFDLHKKPJGJNMF\r\n"
                                "\r\n";

const char test_requ_data[] = "POST /api/sayhi HTTP/1.1\r\n"
"user-agent: vscode-restclient\r\n"
"content-type: application/x-www-form-urlencoded\r\n"
"accept-encoding: gzip, deflate\r\n"
"content-length: 26\r\n"
"Host: 127.0.0.1:8000\r\n"
"Connection: close\r\n"
"\r\n"
"name=foo&age=18&hobby=paly";



std::string data = "------WebKitFormBoundaryKPjN0GYtWEjAni5F\r\n"
                    "Content-Disposition: form-data; name=\"username\"\r\n"
                    "Content-Type: multipart/form-data\r\n"
                    "\r\n"
                    "130533193203240022\r\n"
                    "------WebKitFormBoundaryKPjN0GYtWEjAni5F\r\n"
                    "Content-Disposition: form-data; name=\"password\"\r\n"
                    "Content-Type: multipart/form-data\r\n"
                    "\r\n"
                    "qwerqwer\r\n"
                    "------WebKitFormBoundaryKPjN0GYtWEjAni5F\r\n"
                    "Content-Disposition: form-data; name=\"captchaId\"\r\n"
                    "Content-Type: multipart/form-data\r\n"
                    "\r\n"
                    "img_captcha_7d96b3cd-f873-4c36-8986-584952e38f20\r\n"
                    "------WebKitFormBoundaryKPjN0GYtWEjAni5F\r\n"
                    "Content-Disposition: form-data; name=\"captchaWord\"\r\n"
                    "Content-Type: multipart/form-data\r\n"
                    "\r\n"
                    "rdh5\r\n"
                    "------WebKitFormBoundaryKPjN0GYtWEjAni5F\r\n"
                    "Content-Disposition: form-data; name=\"_csrf\"\r\n"
                    "Content-Type: multipart/form-data\r\n"
                    "\r\n"
                    "200ea95d-90e9-4789-9e0b-435a6dd8b57b\r\n"
                    "------WebKitFormBoundaryKPjN0GYtWEjAni5F--\r\n";

const std::string multi_data = "POST http://www.example.com HTTP/1.1\r\n"
                                "Content-Type:multipart/form-data; boundary=----WebKitFormBoundaryyb1zYhTI38xpQxBK\r\n"
                                "\r\n"
                                "------WebKitFormBoundaryyb1zYhTI38xpQxBK\r\n"
                                "Content-Disposition: form-data; name=\"city_id\"\r\n"
                                "\r\n"
                                "1\r\n"
                                "\r\n"
                                "------WebKitFormBoundaryyb1zYhTI38xpQxBK\r\n"
                                "Content-Disposition: form-data; name=\"company_id\"\r\n"
                                "\r\n"
                                "2\r\n"
                                "------WebKitFormBoundaryyb1zYhTI38xpQxBK\r\n"
                                "Content-Disposition: form-data; name=\"file\"; filename=\"chrome.png\"\r\n"
                                "Content-Type: image/png\r\n"
                                "\r\n"
                                "PNG ... content of chrome.png ...\r\n"
                                "------WebKitFormBoundaryyb1zYhTI38xpQxBK--\r\n";

void test_request() {
    Miren::http::HttpRequestParser parser;
    std::string tmp = test_requ_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    std::cout << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength() << std::endl;

    std::cout << parser.getData()->toString() << std::endl;
    // std::cout << tmp << std::endl << std::endl;
    // std::cout << parser.getData()->getHeader("Content-type") << std::endl;
    auto req = std::move(parser.getData());
    std::cout << (req!=nullptr) << std::endl;
    // std::cout << Miren::http::HttpMethodToString(req->getMethod()) << std::endl;
    std::cout << req->getParamAs<std::string>("name", "s") << std::endl;
    std::cout << req->getParamAs<int>("age", 0) << std::endl;
    std::cout << req->getParamAs<std::string>("hobby", "nihao") << std::endl;

    // std::cout << req->getQuery() << std::endl;
    // std::cout << req->getParam("text") << std::endl;
    // std::cout << req->getParam("sl") << std::endl;
    // std::cout << req->getCookie("JSESSIONID") << std::endl;
    // std::cout << (req->getVersionStr()) << std::endl;
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
    Miren::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size(), true);
    std::cout << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength()
        << " tmp[s]=" << tmp[s] << std::endl;

    tmp.resize(tmp.size() - s);

    std::cout << parser.getData()->toString() << std::endl;
    std::cout << parser.getData()->getHeader("Content-Type") << std::endl;
    std::cout << tmp << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    std::cout << "--------------------------------------------------------" << std::endl;
    // test_response();
    return 0;
}
