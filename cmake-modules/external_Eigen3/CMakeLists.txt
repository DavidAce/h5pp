cmake_minimum_required(VERSION 3.10)
project(external-eigen3)

include(ExternalProject)
ExternalProject_Add(external_eigen3
        URL     https://gitlab.com/libeigen/eigen/-/archive/3.3.7/eigen-3.3.7.tar.gz
        URL_MD5 9e30f67e8531477de4117506fe44669b
        PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/Eigen_3.3.7.patch
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
        )