set(http_SRCS
    HttpSession.cpp
    HttpServer.cpp
    ByteData.cpp
    HttpConnection.cpp
    HttpTcpServer.cpp
    )

add_subdirectory(parser)
add_subdirectory(core)
add_subdirectory(web)

add_library(httpnet ${http_SRCS})
target_link_libraries(httpnet base httpparser httpcore net log)


if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()