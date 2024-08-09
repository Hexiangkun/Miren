add_executable(EchoServer_test EchoServer_test.cpp)
target_link_libraries(EchoServer_test base net log)

add_executable(EchoClient_test EchoClient_test.cpp)
target_link_libraries(EchoClient_test base net log)

if(ZLIB_FOUND)
  add_executable(ZlibStream_test ZlibStream_test.cpp)
  target_link_libraries(ZlibStream_test base net log z)

  add_executable(Zlib_demo Zlib_demo.cpp)
  target_link_libraries(Zlib_demo z)
endif()

if(GTEST_FOUND) 
  SET(TEST_TARGET buffer_unittests)
  ADD_EXECUTABLE(${TEST_TARGET} "")

  TARGET_SOURCES(${TEST_TARGET}
              PRIVATE
              Buffer_test.cpp)
  TARGET_LINK_LIBRARIES(${TEST_TARGET} gtest_main gtest net)
  ENABLE_TESTING()
  ADD_TEST(
    NAME google_test
    COMMAND $<TARGET_FILE:${TEST_TARGET}>)
endif()

add_executable(Channel_test Channel_test.cpp)
target_link_libraries(Channel_test base net log)

add_executable(EventLoop_test EventLoop_test.cpp)
target_link_libraries(EventLoop_test base net log)

add_executable(EventLoopThread_test EventLoopThread_test.cpp)
target_link_libraries(EventLoopThread_test base net log)

add_executable(EventLoopThreadPool_test EventLoopThreadPool_test.cpp)
target_link_libraries(EventLoopThreadPool_test base net log)

