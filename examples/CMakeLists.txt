set(SOURCE_FILES server_main.c)

set(SOURCE_CLI_FILES client_main.c)

add_executable(zRPC_cli ${SOURCE_CLI_FILES})
target_link_libraries(zRPC_cli pthread zRPC)

add_executable(zRPC_serv ${SOURCE_FILES})
target_link_libraries(zRPC_serv pthread zRPC)