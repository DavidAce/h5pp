set(text_tomatch1 "ZLIB/ZLIB-targets.cmake")
set(text_replace1 "zlib/zlib-targets.cmake")
set(text_tomatch2 "//-targets.cmake")
set(text_replace2 "szip/szip-targets.cmake")

set(filename ${HDF5_ROOT}/share/cmake/hdf5/hdf5-config.cmake)
file(READ ${filename} hdf5-config-str)
string(REPLACE "${text_tomatch1}" "${text_replace1}" hdf5-config-tmp  "${hdf5-config-str}")
string(REPLACE "${text_tomatch2}" "${text_replace2}" hdf5-config-str  "${hdf5-config-tmp}")
file(WRITE "${filename}" "${hdf5-config-str}")

