option(HTTP_EXAMPLE_RUN_BENCHMARK "Enable benchmark version of HTTP server example" OFF)

if(HTTP_EXAMPLE_RUN_BENCHMARK)
    add_compile_definitions(__HTTP_EXAMPLE_RUN_BENCHMARK)
endif()

add_subdirectory(server)