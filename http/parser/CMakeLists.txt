set(httpparse_SRCS
    HttpMultipartParser.cpp
    HttpParser.cpp)

add_library(httpparser ${httpparse_SRCS})
target_link_libraries(httpparser jsoncpp log httpnet llhttp_static) 