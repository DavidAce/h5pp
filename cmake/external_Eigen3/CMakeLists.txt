cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)
if(SUPER_CACHEFILE_DIR)
    message(STATUS "Detected external cache file: ${SUPER_CACHEFILE_DIR}")
    load_cache(${SUPER_CACHEFILE_DIR} EXCLUDE CMAKE_INSTALL_PREFIX)
endif()

project(external-eigen3 CXX)
include(ExternalProject)
ExternalProject_Add(external_eigen3
        URL      https://gitlab.com/libeigen/eigen/-/archive/3.4-rc1/eigen-3.4-rc1.tar.gz
        URL_MD5  0839b9721e65d2328fb96eb4290d74cc
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
        # CMake flags
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=${CMAKE_POSITION_INDEPENDENT_CODE}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
        -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=NEVER
        # Eigen3 flags
        -DEIGEN_TEST_CXX11:BOOL=ON
        )