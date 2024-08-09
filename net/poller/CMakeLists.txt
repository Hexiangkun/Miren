set(poll_SRCS
    Poller.cpp
    PollPoller.cpp
    EpollPoller.cpp
    DefaultPoller.cpp)

add_library(poller ${poll_SRCS})
target_link_libraries(poller base net) 