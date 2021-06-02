cmake_minimum_required(VERSION 3.14)
project(external-eigen3)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(ExternalProject)
ExternalProject_Add(external_eigen3
        URL      https://gitlab.com/libeigen/eigen/-/archive/3.4-rc1/eigen-3.4-rc1.tar.gz
        URL_MD5  0839b9721e65d2328fb96eb4290d74cc
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=NEVER
        # Eigen3 flags
        -DEIGEN_TEST_CXX11:BOOL=ON
        -DBUILD_TESTING:BOOL=OFF
        -DEIGEN_BUILD_DOC:BOOL=OFF
        -DEIGEN_BUILD_PKGCONFIG:BOOL=OFF
        )