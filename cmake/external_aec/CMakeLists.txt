cmake_minimum_required(VERSION 3.14)

if(SUPER_CACHEFILE_DIR)
    message(STATUS "Detected external cache file: ${SUPER_CACHEFILE_DIR}")
    load_cache(${SUPER_CACHEFILE_DIR} EXCLUDE CMAKE_INSTALL_PREFIX)
endif()

project(external-aec C CXX)
include(ExternalProject)
ExternalProject_Add(external_aec # Replaces szip
        URL         https://gitlab.dkrz.de/k202009/libaec/uploads/dd3e762a1685e48eefd572e894d24610/libaec-1.0.4.tar.gz
        URL_MD5     f984effda7f969686686384161323253
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
        -DCMAKE_INSTALL_MESSAGE=LAZY
        # AEC flags
        )
