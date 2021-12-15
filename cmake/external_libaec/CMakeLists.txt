cmake_minimum_required(VERSION 3.15)
project(external-szip)
if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()
include(GNUInstallDirs)
include(ExternalProject)

if(MSVC)
    # Fix bug where libaec build gets confused about the shared library names
    set(BUILD_SHARED_LIBS OFF CACHE STRING INTERNAL "")
else()
    # BUILD_SHARED_LIBS ON builds static+shared
    set(BUILD_SHARED_LIBS ON CACHE STRING INTERNAL "")
endif()


ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/CMake-hdf5-1.12.1.tar.gz
        URL_MD5     2191996e136130c57b3508796ce32c2a
        PREFIX      ${CMAKE_BINARY_DIR}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        )

ExternalProject_Add(external_aec
        URL         ${CMAKE_BINARY_DIR}/src/external_hdf5/LIBAEC.tar.gz
        URL_MD5     cb73d4de5388ea5b2647f5eb04a51e11
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        DEPENDS external_hdf5
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        )