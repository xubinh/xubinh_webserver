add_executable(echo_server
    server.cc
)

target_include_directories(echo_server PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(echo_server xubinh_server_library)
