cmake_minimum_required(VERSION 3.19)
project(external-ghc_filesystem)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(ExternalProject)
ExternalProject_Add(external_ghc_filesystem
        URL         https://github.com/gulrak/filesystem/archive/refs/tags/v1.5.6.tar.gz
        URL_MD5     95b3b55c4f5e29f5915a1b21111e043f
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
            ${INIT_CACHE_ARG}
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DCMAKE_INSTALL_MESSAGE=LAZY
            # ghc filesystem flags
            -DGHC_FILESYSTEM_BUILD_TESTING:BOOL=OFF
            -DGHC_FILESYSTEM_BUILD_EXAMPLES:BOOL=OFF
            -DGHC_FILESYSTEM_WITH_INSTALL:BOOL=ON
            )
