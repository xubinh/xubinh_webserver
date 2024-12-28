cmake_minimum_required(VERSION 3.22)

project(XubinhServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(USE_BLOCKING_QUEUE_WITH_RAW_POINTER "Use blocking queue with raw pointer" OFF)
option(USE_LOCK_FREE_QUEUE "Use lock-free queue" OFF)
option(USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER "Use lock-free queue with raw pointer" OFF)

if(USE_LOCK_FREE_QUEUE)
    add_compile_definitions(__USE_LOCK_FREE_QUEUE)

    if(USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER)
        add_compile_definitions(__USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER)
    endif()
else()
    if(USE_BLOCKING_QUEUE_WITH_RAW_POINTER)
        add_compile_definitions(__USE_BLOCKING_QUEUE_WITH_RAW_POINTER)
    endif()
endif()

add_subdirectory(src)

add_subdirectory(example)

option(ENABLE_TESTING "Enable building tests" ON)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
