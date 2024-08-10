#include "http/core/HttpMultipart.h"
#include <iostream>
#include <string>

std::string data = "-----------------------------9051914041544843365972754266\r\n"
                    "Content-Disposition: form-data; name=\"text\"\r\n"
                    "\r\n"
                    "text default\r\n"
                    "-----------------------------9051914041544843365972754266\r\n"
                    "Content-Disposition: form-data; name=\"file2\"; filename=\"a.html\"\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "<!DOCTYPE html><title>Content of a.html.</title>\r\n"
                    "\r\n"
                    "-----------------------------9051914041544843365972754266--\r\n";

std::string test_data = "------WebKitFormBoundaryKPjN0GYtWEjAni5F\r\n"
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

int main()
{
  Miren::http::HttpRequestMultipartBody hm("multipart/form-data; boundary=---------------------------9051914041544843365972754266");
  std::cout << hm.setData(data.c_str(), data.size()) << std::endl;

  auto ml = hm.getFormFile("file2");
  if(ml == nullptr) {
    std::cout << "nullptr" << std::endl;
  }
  std::cout << ml->contentType() << std::endl;

  auto t = hm.getFormValue("text");
  std::cout << t << std::endl;
}