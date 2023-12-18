# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

if (NOT DEFINED ENV{COMPONENTS_PATH})
    message(FATAL_ERROR "You must set 'export COMPONENTS_PATH=<something>' prior to building.")
endif()

message(STATUS "Setting EXTRA_COMPONENT_DIRS=$ENV{COMPONENTS_PATH}")
set(EXTRA_COMPONENT_DIRS $ENV{COMPONENTS_PATH})

add_compile_options(-fmax-errors=5)

project(wifi_client)
