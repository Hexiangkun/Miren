add_executable(HttpParser_test HttpParser_test.cpp)
target_link_libraries(HttpParser_test httpnet)

add_executable(MultipartParser_test MultipartParser_test.cpp)
target_link_libraries(MultipartParser_test httpnet)

add_executable(HttpServer_test HttpServer_test.cpp)
target_link_libraries(HttpServer_test httpnet)

add_executable(HttpWeb_test HttpWeb_test.cpp)
target_link_libraries(HttpWeb_test httpnet httpweb)