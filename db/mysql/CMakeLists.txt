set(sql_SRC
  TmDateTime.cpp
  MysqlConnection.cpp
  MysqlConnectionPool.cpp)
  
add_library(sqlconn ${sql_SRC})
target_link_libraries(sqlconn mysqlclient)

if(NOT CMAKE_BUILD_NO_TESTS)
    add_subdirectory(tests)
endif()