cmake_minimum_required(VERSION 3.15)
project(external-zlib)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)

#ExternalProject_Add(external_hdf5
#        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/CMake-hdf5-1.12.1.tar.gz
#        URL_MD5     2191996e136130c57b3508796ce32c2a
#        PREFIX      ${CMAKE_BINARY_DIR}
#        CONFIGURE_COMMAND ""
#        BUILD_COMMAND ""
#        INSTALL_COMMAND ""
#        )
#ExternalProject_Add(external_zlib
#        URL         ${CMAKE_BINARY_DIR}/src/external_hdf5/ZLib.tar.gz
#        URL_MD5     57164206869101eeadcff9c631cd07ff
#        PREFIX      ${CMAKE_BINARY_DIR}
#        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
#        DEPENDS external_hdf5
#        BUILD_ALWAYS TRUE
#        CMAKE_ARGS
#        ${INIT_CACHE_ARG}
#        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
#        -DBUILD_SHARED_LIBS:BOOL=ON
#        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
#        -DZLIB_EXTERNALLY_CONFIGURED:BOOL=OFF
#        )



ExternalProject_Add(external_zlib
        URL         https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz
        URL_MD5     0095d2d2d1f3442ce1318336637b695f
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        )

#if(NOT BUILD_SHARED_LIBS)
#    ExternalProject_Get_property(external_zlib BINARY_DIR)
#    ExternalProject_Add_Step(external_zlib
#            zlib_remove_shared_${sfx}
#            COMMENT "Remove zlib [${CMAKE_SHARED_LIBRARY_SUFFIX};.dll]"
#            DEPENDEES configure build install
#            COMMAND ${CMAKE_COMMAND} -DMANIFEST:FILEPATH=${BINARY_DIR}/install_manifest.txt -DLIBSUFFIX=.dll$<SEMICOLON>.so$<SEMICOLON>${CMAKE_SHARED_LIBRARY_SUFFIX} -P ${CMAKE_CURRENT_LIST_DIR}/zlib_remove_shared.cmake
#            )
#endif()