cmake_minimum_required (VERSION 2.8.11)
project (PARSEXPR)

set (SRC_PARSEXPR parse_expr.c)
set (HEADER_PARSEXPR parse_expr.h)



add_library (PARSEXPR STATIC ${SRC_PARSEXPR} ${HEADER_PARSEXPR})

target_include_directories(PARSEXPR PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})