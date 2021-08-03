cmake_minimum_required (VERSION 2.8.11)
project (HASH)

set (SRC_HASH hash.c)
set (HEADER_HASH hash.h)



add_library (HASH STATIC ${SRC_HASH} ${HEADER_HASH})

target_include_directories(HASH PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})