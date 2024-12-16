include(FetchContent)

# last release that supports C++11
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/release-1.12.1.zip
)

FetchContent_MakeAvailable(googletest)

file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(xubinh_server_test ${TEST_FILES})

target_link_libraries(xubinh_server_test PRIVATE xubinh_server_library gtest gtest_main)

add_test(NAME XubinhServerTest COMMAND xubinh_server_test)
