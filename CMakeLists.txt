cmake_minimum_required(VERSION 3.31)

project(XubinhWebserver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)

# for enabling recursie parsing
add_subdirectory(src)
add_subdirectory(test)
