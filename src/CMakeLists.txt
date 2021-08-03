cmake_minimum_required (VERSION 2.8.11)
project (AUTO_MAKE)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -flto -Wall -std=c11  -march=native")


add_subdirectory (compression)
add_subdirectory (hash)
add_subdirectory (mmap_utils)
add_subdirectory (parse_expr)
add_subdirectory (str_utils)

add_subdirectory (core)