cmake_minimum_required(VERSION 3.15)
project(external-hdf5)

if(INIT_CACHE_FILE)
    set(INIT_CACHE_ARG -C${INIT_CACHE_FILE})
endif()

include(GNUInstallDirs)
include(ExternalProject)

option(HDF5_ENABLE_PARALLEL         "" OFF)
option(HDF5_ENABLE_Z_LIB_SUPPORT    "" ON)
option(HDF5_ENABLE_SZIP_SUPPORT     "" ON)
option(HDF5_ENABLE_SZIP_ENCODING    "" ON)
option(HDF5_BUILD_HL_LIB            "" ON)
option(HDF5_BUILD_CPP_LIB           "" OFF)
option(HDF5_BUILD_FORTRAN           "" OFF)
option(HDF5_BUILD_JAVA              "" OFF)
option(HDF5_BUILD_TOOLS             "" ON)

set(ZLIB_ROOT    ${CMAKE_INSTALL_PREFIX} CACHE STRING "")
set(SZIP_ROOT    ${CMAKE_INSTALL_PREFIX} CACHE STRING "")

if (HDF5_ENABLE_PARALLEL AND NOT WIN32)
    find_package(MPI COMPONENTS C CXX REQUIRED)
endif ()

ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/CMake-hdf5-1.12.1.tar.gz
        URL_MD5     2191996e136130c57b3508796ce32c2a
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        SOURCE_SUBDIR hdf5-1.12.1
        LIST_SEPARATOR |
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_RPATH:PATH=<INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_SKIP_BUILD_RPATH:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=LAZY
        # HDF5 flags
        -DBUILD_TESTING:BOOL=OFF
        -DHDF_PACKAGE_NAMESPACE:STRING=hdf5::
        -DHDF5_ALLOW_EXTERNAL_SUPPORT:STRING=NO
        -DHDF5_PACKAGE_EXTLIBS:BOOL=ON # So that hdf5-config.cmake includes zlib-config.cmake and szip-config.cmake.
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DHDF5_ENABLE_COVERAGE:BOOL=OFF
        -DHDF5_ENABLE_TRACE:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=${HDF5_ENABLE_Z_LIB_SUPPORT}
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=${HDF5_ENABLE_SZIP_SUPPORT}
        -DHDF5_ENABLE_SZIP_ENCODING:BOOL=${HDF5_ENABLE_SZIP_ENCODING}
        -DZLIB_ROOT:PATH=${ZLIB_ROOT}
        -DSZIP_ROOT:PATH=${SZIP_ROOT}
        -DZLIB_INCLUDE_DIR:PATH=${ZLIB_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}
        -DSZIP_INCLUDE_DIR:PATH=${SZIP_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}
        -DZLIB_DIR:PATH=${ZLIB_ROOT}/share/cmake/zlib
        -DSZIP_DIR:PATH=${SZIP_ROOT}/share/cmake # No subdir
        -DZLIB_PACKAGE_NAME:STRING=zlib
        -DSZIP_PACKAGE_NAME:STRING=szip
        -DSZ_PACKAGE_NAME:STRING=szip
        -DUSE_LIBAEC:BOOL=OFF
        -DZLIB_USE_EXTERNAL:BOOL=OFF
        -DSZIP_USE_EXTERNAL:BOOL=OFF
        -DHDF5_BUILD_TOOLS:BOOL=${HDF5_BUILD_TOOLS}
        -DHDF5_BUILD_HL_LIB:BOOL=${HDF5_BUILD_HL_LIB}
        -DHDF5_BUILD_CPP_LIB:BOOL=${HDF5_BUILD_CPP_LIB}
        -DHDF5_BUILD_FORTRAN:BOOL=${HDF5_BUILD_FORTRAN}
        -DHDF5_BUILD_JAVA:BOOL=${HDF5_BUILD_JAVA}
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_DISABLE_COMPILER_WARNINGS:BOOL=ON
        -DALLOW_UNSUPPORTED=ON
#        --loglevel=DEBUG
        )

# A bug occurs when HDF5 generates hdf5-config.cmake.
# It uses @ZLIB_PACKAGE_NAME@, but this variable gets converted with TOUPPER when calling find_package(ZLIB ...)
# This causes the import path to get hardcoded to something like ZLIB/ZLIB-target.cmake rather than zlib/zlib-target.cmake
# This simply replaces the incorrect string in the generated file

ExternalProject_Add_Step(external_hdf5
        hdf5-config-patch
        COMMENT "Fixing hdf5-config.cmake"
        DEPENDEES configure build install
        COMMAND ${CMAKE_COMMAND} -DHDF5_ROOT:PATH=${CMAKE_INSTALL_PREFIX} -P ${CMAKE_CURRENT_LIST_DIR}/hdf5-config.patch.cmake
        )