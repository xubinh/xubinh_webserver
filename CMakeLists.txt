cmake_minimum_required(VERSION 3.22)

project(XubinhWebserver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src)

add_subdirectory(example)

option(ENABLE_TESTING "Enable building tests" ON)

if(ENABLE_TESTING)
    enable_testing()

    add_subdirectory(test)
endif()
