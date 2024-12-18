add_subdirectory(src)

add_executable(http_server
    server.cc
)

target_include_directories(http_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(http_server http_library xubinh_server_library)
