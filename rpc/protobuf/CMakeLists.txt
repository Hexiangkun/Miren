add_library(miren_protobuf_codec ProtobufCodecLite.cpp)
set_target_properties(miren_protobuf_codec PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(miren_protobuf_codec net protobuf z log)