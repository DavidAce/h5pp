cmake_minimum_required(VERSION 3.10)
project(external-spdlog)
include(../GetNumThreads.cmake)
get_num_threads(NUM_THREADS)
set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${NUM_THREADS})

include(ExternalProject)
ExternalProject_Add(external_spdlog
        URL         https://github.com/gabime/spdlog/archive/v1.4.2.tar.gz
        URL_MD5     373bbb7ea7557cbe6726705463bcae16
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        -DSPDLOG_BUILD_EXAMPLE:BOOL=OFF
        -DSPDLOG_BUILD_TESTS:BOOL=OFF
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
        )
