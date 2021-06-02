cmake_minimum_required(VERSION 3.14)
project(external-fmt)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(ExternalProject)
ExternalProject_Add(external_fmt
        URL         https://github.com/fmtlib/fmt/archive/7.1.3.tar.gz
        URL_MD5     2522ec65070c0bda0ca288677ded2831 # 7.1.3
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
         ${INIT_CACHE_ARG}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
        -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=LAZY
        # fmt flags
        -DFMT_TEST:BOOL=OFF
        -DFMT_DOC:BOOL=OFF
        )
