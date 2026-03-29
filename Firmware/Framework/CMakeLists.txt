cmake_minimum_required(VERSION 3.16)

if(TARGET framework_lib)
    return()
endif()

project(framework C)

file(GLOB_RECURSE fw_sources
    "${CMAKE_CURRENT_LIST_DIR}/API/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/APP/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/Common/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/Driver/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/Libs/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/Utility/*.c"
)

add_library(framework_lib STATIC ${fw_sources})

target_include_directories(framework_lib PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/API"
    "${CMAKE_CURRENT_LIST_DIR}/APP"
    "${CMAKE_CURRENT_LIST_DIR}/Common"
    "${CMAKE_CURRENT_LIST_DIR}/Driver"
    "${CMAKE_CURRENT_LIST_DIR}/Libs"
    "${CMAKE_CURRENT_LIST_DIR}/Utility"
)
