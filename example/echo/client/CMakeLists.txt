add_subdirectory(src)

add_executable(echo_client
    client.cc
)

target_include_directories(echo_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(echo_client echo_library xubinh_server_library)
