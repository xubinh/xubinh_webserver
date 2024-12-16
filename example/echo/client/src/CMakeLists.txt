# gather source files
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

add_library(echo_library STATIC ${SRC_FILES})

target_include_directories(echo_library PUBLIC
    ../include
    ${PROJECT_SOURCE_DIR}/include
)
