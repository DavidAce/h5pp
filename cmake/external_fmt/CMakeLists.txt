cmake_minimum_required(VERSION 3.15)
project(external-fmt)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)
ExternalProject_Add(external_fmt
        URL         https://github.com/fmtlib/fmt/archive/7.1.3.tar.gz
        URL_MD5     2522ec65070c0bda0ca288677ded2831 # 7.1.3
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
         ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        # fmt flags
        -DFMT_TEST:BOOL=OFF
        -DFMT_DOC:BOOL=OFF
        )
