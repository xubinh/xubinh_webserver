file(GLOB APP_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} */)

# add all sub folders (assuming each contains one application) using relative path
foreach(APP_DIR ${APP_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${APP_DIR})
        add_subdirectory(${APP_DIR})
    endif()
endforeach()
