cmake_minimum_required(VERSION 3.9)
project(imp C)
set(CMAKE_C_STANDARD 11)
set (IMP_VERSION_MAJOR 1)
set (IMP_VERSION_MINOR 0)

configure_file (
        "${PROJECT_SOURCE_DIR}/ImpConfig.h.in"
        "${PROJECT_BINARY_DIR}/ImpConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}")

add_subdirectory(httplib)
include_directories("${PROJECT_SOURCE_DIR}/httplib")
add_executable(httplib_test httplib/tests/httplib_test.c)
target_link_libraries(httplib_test httplib cmocka)


add_executable(imp main.c)
target_link_libraries(imp httplib)
