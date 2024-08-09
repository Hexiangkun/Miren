set(http_SRCS
  HttpServer.cpp
  HttpResponse.cpp
  HttpContext.cpp
  )

add_library(example_http ${http_SRCS})
target_link_libraries(example_http net log)

if(NOT CMAKE_BUILD_NO_TESTS)
  add_subdirectory(tests)
endif()