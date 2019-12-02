cmake_minimum_required(VERSION 3.10)
project(external-eigen3)

include(ExternalProject)
ExternalProject_Add(external_eigen3
        GIT_REPOSITORY https://github.com/eigenteam/eigen-git-mirror.git
        GIT_TAG 3.3.7
        GIT_PROGRESS 1
        PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/Eigen_3.3.7.patch
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)