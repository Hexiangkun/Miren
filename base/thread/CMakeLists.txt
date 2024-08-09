set(base_thread_SRCS
    Condition.cpp
    CountDownLatch.cpp
    CurrentThread.cpp
    Thread.cpp
    ThreadPool.cpp)

add_library(base_thread ${base_thread_SRCS})

target_link_libraries(base_thread pthread)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()