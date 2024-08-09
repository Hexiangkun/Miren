set(http_SRCS
    HttpCode.cpp
    HttpMultipart.cpp
    HttpRequest.cpp
    HttpResponse.cpp)

add_library(httpcore ${http_SRCS})
target_link_libraries(httpcore base log httpparser)