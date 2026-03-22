cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE app_sources
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

set(app_include_dirs
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/Config"
)

if(NOT CMAKE_SCRIPT_MODE_FILE AND NOT TARGET framework_lib)
    add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../Framework"
        "${CMAKE_BINARY_DIR}/framework_build"
    )
endif()

idf_component_register(
    SRCS         ${app_sources}
    INCLUDE_DIRS ${app_include_dirs}
    REQUIRES     driver freertos esp_common
)

if(TARGET framework_lib)
    idf_build_get_property(idf_include_dirs BUILD_INCLUDE_DIRECTORIES)
    target_include_directories(framework_lib PRIVATE ${idf_include_dirs})

    target_link_libraries(framework_lib PRIVATE
        idf::driver
        idf::freertos
        idf::esp_common
    )

    target_compile_definitions(framework_lib PUBLIC
        PROJECT_CONFIG_H="project_config.h"
    )

    target_include_directories(framework_lib PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/Config"
    )

    target_link_libraries(${COMPONENT_LIB} PUBLIC framework_lib)
endif()