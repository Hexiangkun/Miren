add_executable(TryVoid_test TryVoid_test.cpp)
target_link_libraries(TryVoid_test future)

if(GTEST_FOUND)
  SET(CALL_TARGET call_unittest)
  SET(THREADPOOL_TARGET threadPool_unittest)
  ADD_EXECUTABLE(${CALL_TARGET} "")
  ADD_EXECUTABLE(${THREADPOOL_TARGET} "")

  TARGET_SOURCES(${CALL_TARGET}
              PRIVATE
              CallUnit_Test.cpp)
  TARGET_SOURCES(${THREADPOOL_TARGET}
              PRIVATE
              ThreadPool_test.cpp)

  TARGET_LINK_LIBRARIES(${CALL_TARGET} gtest_main gtest future)
  TARGET_LINK_LIBRARIES(${THREADPOOL_TARGET} gtest_main gtest future)

  ENABLE_TESTING()
  ADD_TEST(
    NAME google_test
    COMMAND $<TARGET_FILE:${CALL_TARGET} 
                          ${THREADPOOL_TARGET}>
    )

endif()

