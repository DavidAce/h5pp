cmake_minimum_required(VERSION 3.15)
project(external-spdlog)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)
ExternalProject_Add(external_spdlog
#        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz
#        URL_MD5     287c6492c25044fd2da9947ab120b2bd #1.11.0
        URL_MD5     6b4446526264c1d1276105482adc18d1 #1.11.0
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_POLICY_DEFAULT_CMP0074=NEW
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        -DSPDLOG_BUILD_EXAMPLE:BOOL=OFF
        -DSPDLOG_BUILD_TESTS:BOOL=OFF
        -DSPDLOG_BUILD_BENCH:BOOL=OFF
        -DSPDLOG_FMT_EXTERNAL:BOOL=${SPDLOG_FMT_EXTERNAL}
        -Dfmt_ROOT:PATH=${fmt_ROOT}
        )
