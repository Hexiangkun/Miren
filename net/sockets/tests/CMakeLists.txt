if(GTEST_FOUND) 
  SET(TEST_TARGET inetaddress_unittests)
  ADD_EXECUTABLE(${TEST_TARGET} "")

  TARGET_SOURCES(${TEST_TARGET}
              PRIVATE
              InetAddress_test.cpp)
  TARGET_LINK_LIBRARIES(${TEST_TARGET} gtest_main gtest net)
  ENABLE_TESTING()
  ADD_TEST(
    NAME google_test
    COMMAND $<TARGET_FILE:${TEST_TARGET}>)
endif()