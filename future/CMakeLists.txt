set(future_SRCS
    ThreadPool.cpp)

add_library(future ${future_SRCS})

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()