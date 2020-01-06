cmake_minimum_required(VERSION 3.10)
project(external-eigen3)
include(../GetNumThreads.cmake)
get_num_threads(NUM_THREADS)
set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${NUM_THREADS})
include(ExternalProject)
ExternalProject_Add(external_eigen3
        GIT_REPOSITORY https://github.com/eigenteam/eigen-git-mirror.git
        GIT_TAG 3.3.7
        GIT_PROGRESS 1
        GIT_SHALLOW 1
        PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/Eigen_3.3.7.patch
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        -DEIGEN_TEST_CXX11:BOOL=ON
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
        )