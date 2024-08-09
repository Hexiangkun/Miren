set(redis_SRCS
    RedisConn.cpp
    RedisPubSub.cpp)

add_library(redisconn ${redis_SRCS})
target_link_libraries(redisconn hiredis log)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()