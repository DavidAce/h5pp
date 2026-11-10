cmake_minimum_required(VERSION 3.26)
project(external-zlib)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)

# Static zlib may be linked into a shared HDF5 library.
option(CMAKE_POSITION_INDEPENDENT_CODE "Build position-independent libraries" ON)

ExternalProject_Add(external_zlib
#        URL         https://github.com/madler/zlib/archive/refs/tags/v1.2.12.tar.gz
#        URL         https://github.com/madler/zlib/archive/refs/tags/v1.3.tar.gz
        URL         https://github.com/madler/zlib/archive/refs/tags/v1.4.tar.gz
#        URL_MD5     db5b7326d4e0dbcbd1981b640d495c9b # v1.2.12
#        URL_MD5     c3d6da426165d651fd3bddef29406928 # v1.3
        URL_MD5     67a96ddfe1b7b0193f585ccc0770903e # v1.4
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        USES_TERMINAL_DOWNLOAD TRUE
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
        USES_TERMINAL_INSTALL TRUE
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=${CMAKE_POSITION_INDEPENDENT_CODE}
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        )
