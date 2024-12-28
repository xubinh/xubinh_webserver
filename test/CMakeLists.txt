function(snake_to_camel INPUT_STRING OUTPUT_STRING)
    # e.g. "log_file" -> ["log", "file"]
    string(REGEX MATCHALL "[a-zA-Z0-9]+" WORDS ${INPUT_STRING})

    set(CAMEL_CASE_STRING "")

    # e.g. "" -> "Log" -> "LogFile"
    foreach(WORD ${WORDS})
        string(SUBSTRING ${WORD} 0 1 FIRST_LETTER) # e.g. "log" -> "l"
        string(SUBSTRING ${WORD} 1 -1 REST) # e.g. "log" -> "og"
        string(TOUPPER ${FIRST_LETTER} FIRST_LETTER_UPPER) # e.g. "l" -> "L"
        string(CONCAT CAMEL_CASE_STRING ${CAMEL_CASE_STRING} ${FIRST_LETTER_UPPER}${REST}) # e.g. "" -> "Log"
    endforeach()

    set(${OUTPUT_STRING} ${CAMEL_CASE_STRING} PARENT_SCOPE)
endfunction()

include(FetchContent)

# last release that supports C++11
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/release-1.12.1.zip
)

FetchContent_MakeAvailable(googletest)

file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

foreach(TEST_FILE ${TEST_FILES})
    # get file name without extension
    get_filename_component(EXECUTABLE_NAME ${TEST_FILE} NAME_WE)

    add_executable(${EXECUTABLE_NAME} ${TEST_FILE})

    target_link_libraries(${EXECUTABLE_NAME} PRIVATE xubinh_server_library gtest gtest_main)

    snake_to_camel(${EXECUTABLE_NAME} EXECUTABLE_NAME_IN_CAMEL_CASE)

    add_test(NAME ${EXECUTABLE_NAME_IN_CAMEL_CASE}Test COMMAND ${EXECUTABLE_NAME})
endforeach()