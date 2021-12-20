set(text_tomatch1 "set (LINK_COMP_LIBS \${LINK_COMP_LIBS} \${ZLIB_LIBRARIES})")
set(text_replace1 "message(\"Found ZLIB: \${ZLIB_LIBRARIES}\")")
set(text_tomatch2 "set (LINK_COMP_LIBS \${LINK_COMP_LIBS} \${SZIP_LIBRARIES})")
set(text_replace2 "message(\"Found SZIP: \${SZIP_LIBRARIES}\")")

set(filename ${CMAKE_CURRENT_SOURCE_DIR}/CMakeFilters.cmake)
file(READ ${filename} cmakelists-txt)
string(REPLACE "${text_tomatch1}" "${text_replace1}" cmakelists-tmp  "${cmakelists-txt}")
string(REPLACE "${text_tomatch2}" "${text_replace2}" cmakelists-txt  "${cmakelists-tmp}")
file(WRITE "${filename}" "${cmakelists-txt}")

