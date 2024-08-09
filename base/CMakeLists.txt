set(base_SRCS
    Date.cpp
    ErrorInfo.cpp
    Exception.cpp
    FileUtil.cpp
    ProcessInfo.cpp
    StringUtil.cpp
    Timestamp.cpp
    TimeZone.cpp
    Util.cpp)

add_library(base ${base_SRCS})
target_link_libraries(base pthread log)

add_subdirectory(thread)
add_subdirectory(log)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()