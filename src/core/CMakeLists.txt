cmake_minimum_required (VERSION 2.8.11)
project (SEARCH_CORE)

set (SRC_JSON_PARSE wiki_json_parse.c)
set (POS_SRC positional_index.c)
set (COMPR_INDX_SRC compress_index.c)
set (QUERY_COMPRESSED_INDEX_SRC query_compressed_index.c)


add_executable(parse ${SRC_JSON_PARSE})
target_link_libraries(parse PUBLIC UTF8UTILS HASH STEMMING)


add_executable(index ${POS_SRC})
target_link_libraries(index PUBLIC UTF8UTILS HASH MMAPUTILS)


add_executable(compress ${COMPR_INDX_SRC})
target_link_libraries(compress PUBLIC MMAPUTILS COMPRESSION)


add_executable(query ${QUERY_COMPRESSED_INDEX_SRC})
target_link_libraries(query PUBLIC m UTF8UTILS UTF8UTILS HASH MMAPUTILS PARSEXPR COMPRESSION STEMMING)