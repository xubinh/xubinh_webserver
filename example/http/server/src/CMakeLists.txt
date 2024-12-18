file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

add_library(http_library STATIC ${SRC_FILES})

target_include_directories(http_library PUBLIC
    ../include
    ${PROJECT_SOURCE_DIR}/include
)
