# gather source files
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

# define a new lib representing the whole project
add_library(webserver STATIC ${SRC_FILES})

# platform-independent way of dependency resolving
find_package(Threads REQUIRED)
target_link_libraries(webserver PUBLIC Threads::Threads)

target_include_directories(webserver PUBLIC ${PROJECT_SOURCE_DIR}/include)
