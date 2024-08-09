add_executable(LogFile_test LogFile_test.cpp)
target_link_libraries(LogFile_test log)

add_executable(AsyncLogging_test AsyncLogging_test.cpp)
target_link_libraries(AsyncLogging_test log)


add_executable(LogStream_bench LogStream_bench.cpp)
target_link_libraries(LogStream_bench log)


add_executable(Logging_test Logging_test.cpp)
target_link_libraries(Logging_test log)


if(GTEST_FOUND)
  SET(TEST_TARGET logstream_unittests)
  ADD_EXECUTABLE(${TEST_TARGET} "")

  TARGET_SOURCES(${TEST_TARGET}
              PRIVATE
              LogStream_test.cpp)
  TARGET_LINK_LIBRARIES(${TEST_TARGET} gtest_main gtest log)
  ENABLE_TESTING()
  ADD_TEST(
    NAME google_test
    COMMAND $<TARGET_FILE:${TEST_TARGET}>)
endif()