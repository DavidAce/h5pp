cmake_minimum_required(VERSION 3.14)
project(external-zlib)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(ExternalProject)
ExternalProject_Add(external_zlib
        URL         https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz
        URL_MD5     0095d2d2d1f3442ce1318336637b695f
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=LAZY
        )


