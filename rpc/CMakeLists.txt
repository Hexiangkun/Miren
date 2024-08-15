add_subdirectory(protobuf)

add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS rpc.proto
  VERBATIM )

set_source_files_properties(rpc.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion")
include_directories(${PROJECT_BINARY_DIR})

add_library(miren_protorpc_wire rpc.pb.cc RpcCodec.cpp)
set_target_properties(miren_protorpc_wire PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")


add_library(miren_protorpc RpcChannel.cpp RpcServer.cpp)
set_target_properties(miren_protorpc PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(miren_protorpc miren_protorpc_wire miren_protobuf_codec net protobuf z)

if(TCMALLOC_LIBRARY)
  target_link_libraries(miren_protorpc tcmalloc_and_profiler)
endif()

if(NOT CMAKE_BUILD_NO_TESTS)
  add_subdirectory(tests)
endif()