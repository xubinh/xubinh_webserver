add_subdirectory(src)

add_executable(client
    client.cc
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(client echo_library xubinh_server_library)
