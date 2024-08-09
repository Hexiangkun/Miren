set(log_SRCS
    AsyncLogging.cpp
    LogFile.cpp
    Logging.cpp
    LogStream.cpp)

add_library(log ${log_SRCS})
target_link_libraries(log base base_thread)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()