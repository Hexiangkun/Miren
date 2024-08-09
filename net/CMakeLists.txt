set(net_SRCS
    Acceptor.cpp
    Buffer.cpp
    Channel.cpp
    Connector.cpp
    EventLoop.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    TcpConnection.cpp
    TcpClient.cpp
    TcpServer.cpp)

add_subdirectory(sockets)
add_subdirectory(poller)
add_subdirectory(timer)
add_subdirectory(udp)

add_library(net ${net_SRCS})
target_link_libraries(net base log poller sockets timer)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()