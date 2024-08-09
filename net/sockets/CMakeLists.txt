set(sockets_SRCS
    InetAddress.cpp
    Socket.cpp
    SocketsOps.cpp)

add_library(sockets ${sockets_SRCS})
target_link_libraries(sockets base log)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()
