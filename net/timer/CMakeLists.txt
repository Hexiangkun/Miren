set(time_SRCS
    Timer.cpp
    TimerQueue.cpp)

add_library(timer ${time_SRCS})
target_link_libraries(timer base net)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()