add_executable(server
    server.cc
)

target_include_directories(server PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(server xubinh_server_library)
