cmake_minimum_required(VERSION 3.15)
project(external-zlib)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)

ExternalProject_Add(external_zlib
#        URL         https://github.com/madler/zlib/archive/refs/tags/v1.2.12.tar.gz
        URL         https://github.com/madler/zlib/archive/refs/tags/v1.3.tar.gz
#        URL_MD5     db5b7326d4e0dbcbd1981b640d495c9b # v1.2.12
        URL_MD5     c3d6da426165d651fd3bddef29406928 # v1.3
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        )
