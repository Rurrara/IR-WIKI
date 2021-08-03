cmake_minimum_required (VERSION 2.8.11)
project (STR_UTILS)


set (SRC_UTF8UTILS Utf8Func.c)
set (HEADER_UTF8UTILS Utf8Func.h)

set (SRC_STEMMING stemming.c)
set (HEADER_STEMMING stemming.h)

add_library (UTF8UTILS STATIC ${SRC_UTF8UTILS} ${HEADER_UTF8UTILS})
add_library (STEMMING STATIC ${SRC_STEMMING} ${HEADER_STEMMING})

target_include_directories(UTF8UTILS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(STEMMING PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})



