cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (MAD)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic -g -O0")

include_directories (${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
