cmake_minimum_required (VERSION 2.8.11)
project (COMPRESSION)

set (SRC_COMPRESSION compression.c)
set (HEADER_COMPRESSION compression.h)



add_library (COMPRESSION STATIC ${SRC_COMPRESSION} ${HEADER_COMPRESSION})

target_include_directories(COMPRESSION PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})