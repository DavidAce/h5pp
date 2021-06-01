cmake_minimum_required(VERSION 3.14)

if(SUPER_CACHEFILE_DIR)
    message(STATUS "Detected external cache file: ${SUPER_CACHEFILE_DIR}")
    load_cache(${SUPER_CACHEFILE_DIR} EXCLUDE CMAKE_INSTALL_PREFIX)
endif()

project(external-fmt CXX)
include(ExternalProject)
ExternalProject_Add(external_fmt
        URL         https://github.com/fmtlib/fmt/archive/7.1.3.tar.gz
        URL_MD5     2522ec65070c0bda0ca288677ded2831 # 7.1.3
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=${CMAKE_POSITION_INDEPENDENT_CODE}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
        -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=NEVER
        # fmt flags
        -DFMT_TEST:BOOL=${FMT_TEST}
        -DFMT_DOC:BOOL=${FMT_DOC}
        )
