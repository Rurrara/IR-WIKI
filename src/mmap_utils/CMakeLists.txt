cmake_minimum_required (VERSION 2.8.11)
project (MMAP_UTILS)


set (SRC_MMAPUTILS mmap_utils.c)
set (HEADER_MMAPUTILS mmap_utils.h)


add_library (MMAPUTILS STATIC ${SRC_MMAPUTILS} ${HEADER_MMAPUTILS})

target_include_directories(MMAPUTILS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

