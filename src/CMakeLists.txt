option(USE_BLOCKING_QUEUE_WITH_RAW_POINTER "Use blocking queue with raw pointer" OFF)
option(USE_LOCK_FREE_QUEUE "Use lock-free queue" OFF)
option(USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER "Use lock-free queue with raw pointer" OFF)

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

option(USE_SHARED_PTR_DESTRUCTION_TRANSFERING "Use `std::shared_ptr` destruction transfering" OFF)

if(USE_SHARED_PTR_DESTRUCTION_TRANSFERING)
    add_compile_definitions(__USE_SHARED_PTR_DESTRUCTION_TRANSFERING)
endif()

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

add_library(xubinh_server_library STATIC ${SRC_FILES})

find_package(Threads REQUIRED)
target_link_libraries(xubinh_server_library PUBLIC Threads::Threads)

target_include_directories(xubinh_server_library PUBLIC ${PROJECT_SOURCE_DIR}/include)
