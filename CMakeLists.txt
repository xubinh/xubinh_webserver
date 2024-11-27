cmake_minimum_required(VERSION 3.22)

project(XubinhWebserver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)

# for enabling recursive parsing
add_subdirectory(src)

enable_testing()

add_subdirectory(test)