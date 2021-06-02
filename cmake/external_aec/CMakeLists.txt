cmake_minimum_required(VERSION 3.14)
project(external-aec)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(ExternalProject)
ExternalProject_Add(external_aec # Replaces szip
        URL         https://gitlab.dkrz.de/k202009/libaec/uploads/dd3e762a1685e48eefd572e894d24610/libaec-1.0.4.tar.gz
        URL_MD5     f984effda7f969686686384161323253
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=LAZY
        )
