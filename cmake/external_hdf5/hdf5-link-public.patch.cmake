set(text_tomatch1 "PRIVATE \${LINK_LIBS} \${LINK_COMP_LIBS}")
set(text_replace1 "PRIVATE")
set(text_tomatch2 "PUBLIC $<$<NOT:$<PLATFORM_ID:Windows>>")
set(text_replace2 "PUBLIC \${LINK_LIBS} \${LINK_COMP_LIBS} $<$<NOT:$<PLATFORM_ID:Windows>>")



set(filename ${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt)

file(READ ${filename} cmakelists-txt)
string(REPLACE "${text_tomatch1}" "${text_replace1}" cmakelists-tmp  "${cmakelists-txt}")
string(REPLACE "${text_tomatch2}" "${text_replace2}" cmakelists-txt  "${cmakelists-tmp}")
file(WRITE "${filename}" "${cmakelists-txt}")

set(text_tomatch3 "set (LINK_COMP_LIBS \${LINK_COMP_LIBS} \${ZLIB_LIBRARIES})")
set(text_replace3 "message(\"Found ZLIB: \${ZLIB_LIBRARIES}\")")