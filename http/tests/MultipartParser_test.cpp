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

  
  std::string m = "name=foo&age=18&hobby=paly";
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
  std::cout <<  m.size() << std::endl;
}