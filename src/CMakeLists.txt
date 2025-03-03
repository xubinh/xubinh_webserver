file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

add_library(xubinh_server_library STATIC ${SRC_FILES})

find_package(Threads REQUIRED)
target_link_libraries(xubinh_server_library PUBLIC Threads::Threads)
target_link_libraries(xubinh_server_library PUBLIC atomic)

target_include_directories(xubinh_server_library PUBLIC ${PROJECT_SOURCE_DIR}/include)
