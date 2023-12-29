cmake_minimum_required(VERSION 3.15)
project(external-szip)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)


ExternalProject_Add(external_aec
        URL         https://gitlab.dkrz.de/k202009/libaec/-/archive/v1.1.2/libaec-v1.1.2.tar.gz
        URL_MD5     efc656b501fe832fc18fa464ca4b93c8
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        -DBUILD_SHARED_LIBS:BOOL=OFF
        )