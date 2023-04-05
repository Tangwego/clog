cmake_minimum_required(VERSION 3.23)
project(clog C)

set(CMAKE_C_STANDARD 99)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(log STATIC log.c)
add_executable(clog main.c)

target_link_libraries(clog log)