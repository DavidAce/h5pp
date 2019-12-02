cmake_minimum_required(VERSION 3.10)
project(external-spdlog)

include(ExternalProject)
ExternalProject_Add(external_spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.3.1
        GIT_PROGRESS 1
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DSPDLOG_BUILD_EXAMPLES:BOOL=OFF
        -DSPDLOG_BUILD_BENCH:BOOL=OFF
        -DSPDLOG_BUILD_TESTS:BOOL=OFF
        )
