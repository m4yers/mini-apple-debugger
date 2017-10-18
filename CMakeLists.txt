cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (MAD)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

include_directories (${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)


enable_testing()
add_subdirectory(test)
