
aux_source_directory(./model CHAT_MODEL_LIST)
aux_source_directory(./control CHAT_CONTROL_LIST)
aux_source_directory(./service CHAT_SERVICE_LIST)
aux_source_directory(./server CHAT_SERVER_LIST)

add_executable(chatServer   ServerMain.cpp
        ${CHAT_MODEL_LIST}        ${CHAT_CONTROL_LIST}
        ${CHAT_SERVICE_LIST}         ${CHAT_SERVER_LIST})

add_executable(chatClient   ClientMain.cpp  ${CHAT_MODEL_LIST})

target_link_libraries(chatServer sqlconn net log redisconn)