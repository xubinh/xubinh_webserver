enable_testing()

include(FetchContent)

# last release that supports C++11
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/release-1.12.1.zip
)

FetchContent_MakeAvailable(googletest)

# gather test files
file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(webserver_test ${TEST_FILES})

target_link_libraries(webserver_test PRIVATE webserver gtest gtest_main)

add_test(NAME WebserverTest COMMAND webserver_test)
