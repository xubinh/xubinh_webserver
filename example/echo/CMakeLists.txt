# add_executable(echo_server server.cc)
# target_include_directories(echo_server PRIVATE ${PROJECT_SOURCE_DIR}/include)
# target_link_libraries(echo_server PRIVATE webserver)

# add_executable(echo_client client.cc)
# target_include_directories(echo_client PRIVATE ${PROJECT_SOURCE_DIR}/include)
# target_link_libraries(echo_client PRIVATE webserver)

add_subdirectory(client)
add_subdirectory(server)