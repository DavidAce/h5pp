cmake_minimum_required(VERSION 3.15)
project(external-spdlog)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)
ExternalProject_Add(external_spdlog
#        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.3.1.tar.gz
#        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.5.0.tar.gz
#        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.8.0.tar.gz
#        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.9.2.tar.gz
#        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.10.0.tar.gz
        URL         https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
#        URL_MD5     3c17dd6983de2a66eca8b5a0b213d29f #1.3.1
#        URL_MD5     a966eea01f81551527853d282896cb4d #1.5.0
#        URL_MD5     cbd179161d1ed185bd9f3f242c424fd7 #1.8.0
#        URL_MD5     cee7f3d31178a00791d7a22c6738df6d #1.9.2
#        URL_MD5     effea813cd81cfb5588806c5754e14f1 #1.10.0
        URL_MD5     287c6492c25044fd2da9947ab120b2bd #1.11.0
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
