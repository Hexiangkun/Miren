cmake_minimum_required(VERSION 3.22.0)

project(server)

set(CMAKE_CXX_STANDARD 17)

enable_testing()

 if(NOT CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE "Release")
 endif()
set(CXX_FLAGS
 -g
# -DVALGRIND
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Wno-unused-parameter
#  -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wimplicit-fallthrough
 -Wshadow
 -Wwrite-strings
 -march=native
 -MMD
 -std=c++17
 -rdynamic
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

#message("C++ 编译选项: ${CMAKE_CXX_FLAGS}")

include_directories(${PROJECT_SOURCE_DIR})

find_package(ZLIB)
find_package(Protobuf)
find_package(GTest REQUIRED)
if(ZLIB_FOUND)
    message(STATUS "found zlib")
endif()
if(PROTOBUF_FOUND) 
    message(STATUS "found protobuf")
endif()
if(GTEST_FOUND) 
    message(STATUS "found gtest")
endif()

add_subdirectory(base)
add_subdirectory(config)
add_subdirectory(net)

add_subdirectory(third_party/kcpp)
add_subdirectory(third_party/llhttp)
add_subdirectory(http)
add_subdirectory(rpc)
add_subdirectory(future)
add_subdirectory(example)
add_subdirectory(db)