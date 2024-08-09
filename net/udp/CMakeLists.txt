set(udp_SRCS
    UdpAcceptor.cpp
    UdpClient.cpp
    UdpConnection.cpp
    UdpConnector.cpp
    UdpServer.cpp)


add_library(udp ${udp_SRCS})
target_link_libraries(udp base net kcpp)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()