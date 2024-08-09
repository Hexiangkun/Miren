add_executable(TimeStamp_test TimeStamp_test.cpp)
target_link_libraries(TimeStamp_test base)

add_executable(Singleton_test Singleton_test.cpp)
target_link_libraries(Singleton_test base base_thread)

add_executable(ProcessInfo_test ProcessInfo_test.cpp)
target_link_libraries(ProcessInfo_test base base_thread)

add_executable(FileUtil_test FileUtil_test.cpp)
target_link_libraries(FileUtil_test base)

add_executable(Exception_test Exception_test.cpp)
target_link_libraries(Exception_test base_thread base)

add_executable(Date_test Date_test.cpp)
target_link_libraries(Date_test base)

if(ZLIB_FOUND)
    add_executable(GzipFile_test GzipFile_test.cpp)
    target_link_libraries(GzipFile_test base z)
endif()

add_executable(BlockQueue_test BlockQueue_test.cpp)
target_link_libraries(BlockQueue_test base base_thread)

add_executable(BoundBlockQueue_test BoundBlockQueue_test.cpp)
target_link_libraries(BoundBlockQueue_test base base_thread)

add_executable(Fork_test Fork_test.cpp)
target_link_libraries(Fork_test base base_thread)

if(GTEST_FOUND)
  SET(TEST_TARGET Base64_test)
  ADD_EXECUTABLE(${TEST_TARGET} "")

  TARGET_SOURCES(${TEST_TARGET}
              PRIVATE
              Base64_test.cpp)
  TARGET_LINK_LIBRARIES(${TEST_TARGET} gtest_main gtest base)
  ENABLE_TESTING()
  ADD_TEST(
    NAME google_test
    COMMAND $<TARGET_FILE:${TEST_TARGET}>)
endif()