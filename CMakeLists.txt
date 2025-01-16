cmake_minimum_required(VERSION 3.22)

project(XubinhServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wconversion -Wshadow")

# set different configs for different build types
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

# set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

option(ENABLE_TEST "Enable building tests" ON)
option(USE_LOCK_FREE_QUEUE "Use lock-free queue" OFF)
option(USE_BLOCKING_QUEUE_WITH_RAW_POINTER "Use blocking queue with raw pointer" OFF)
option(USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER "Use lock-free queue with raw pointer" OFF)
option(USE_SHARED_PTR_DESTRUCTION_TRANSFERING "Use `std::shared_ptr` destruction transfering" OFF)

if(NOT USE_LOCK_FREE_QUEUE)
    if(USE_BLOCKING_QUEUE_WITH_RAW_POINTER)
        add_compile_definitions(__USE_BLOCKING_QUEUE_WITH_RAW_POINTER)
    endif()
else()
    add_compile_definitions(__USE_LOCK_FREE_QUEUE)

    if(USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER)
        add_compile_definitions(__USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER)
    endif()
endif()

if(USE_SHARED_PTR_DESTRUCTION_TRANSFERING)
    add_compile_definitions(__USE_SHARED_PTR_DESTRUCTION_TRANSFERING)
endif()

add_subdirectory(src)
add_subdirectory(example)

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
