set(web_SRCS
  HttpContext.cpp
  HttpRouter.cpp
  HttpWeb.cpp)

add_library(httpweb ${web_SRCS})
target_link_libraries(httpweb httpnet)