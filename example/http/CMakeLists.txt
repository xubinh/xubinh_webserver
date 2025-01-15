option(RUN_BENCHMARK "Run benchmark" OFF)

if(RUN_BENCHMARK)
    add_compile_definitions(__RUN_BENCHMARK)
endif()

add_subdirectory(server)