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
option(HDF5_ENABLE_SZIP_SUPPORT     "" OFF)
option(HDF5_ENABLE_SZIP_SUPPORT     "" OFF)
option(HDF5_BUILD_FORTRAN           "" OFF)
option(HDF5_BUILD_JAVA              "" OFF)


if(HDF5_ENABLE_SZIP_SUPPORT OR HDF5_ENABLE_Z_LIB_SUPPORT)
    set(HDF5_ALLOW_EXTERNAL_SUPPORT TGZ)
else()
    set(HDF5_ALLOW_EXTERNAL_SUPPORT OFF)
endif()
if (HDF5_ENABLE_PARALLEL AND NOT WIN32)
    find_package(MPI COMPONENTS C CXX REQUIRED)
endif ()

if(HDF5_ENABLE_Z_LIB_SUPPORT)
    ExternalProject_Add(external_zlib-download
            URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/CMake-hdf5-1.12.1.tar.gz
            URL_MD5     2191996e136130c57b3508796ce32c2a
            PREFIX      ${CMAKE_BINARY_DIR}
            SOURCE_DIR  src/external_hdf5 # So that external_hdf5 doesn't have to download the file again.
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            )
    ExternalProject_Add(external_zlib
            URL         ${CMAKE_BINARY_DIR}/src/external_hdf5/ZLib.tar.gz
            URL_MD5     57164206869101eeadcff9c631cd07ff
            PREFIX      ${CMAKE_BINARY_DIR}
            INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
            DEPENDS external_zlib-download
            BUILD_ALWAYS TRUE
            CMAKE_ARGS
            ${INIT_CACHE_ARG}
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DBUILD_SHARED_LIBS:BOOL=ON
            -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
            -DZLIB_EXTERNALLY_CONFIGURED:BOOL=OFF
            )
    list(APPEND hdf5_depends external_zlib)
endif()
if(HDF5_ENABLE_SZIP_SUPPORT)
    ExternalProject_Add(external_aec-download
            URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/CMake-hdf5-1.12.1.tar.gz
            URL_MD5     2191996e136130c57b3508796ce32c2a
            PREFIX      ${CMAKE_BINARY_DIR}
            SOURCE_DIR  src/external_hdf5 # So that external_hdf5 doesn't have to download the file again.
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            )
    ExternalProject_Add(external_aec
            URL         ${CMAKE_BINARY_DIR}/src/external_hdf5/LIBAEC.tar.gz
            URL_MD5     cb73d4de5388ea5b2647f5eb04a51e11
            PREFIX      ${CMAKE_BINARY_DIR}
            INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
            DEPENDS external_aec-download
            BUILD_ALWAYS TRUE
            CMAKE_ARGS
            ${INIT_CACHE_ARG}
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DBUILD_SHARED_LIBS:BOOL=ON
            -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
            )
    list(APPEND hdf5_depends external_aec)
endif()

unset(ZLIB_FOUND CACHE)
ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/CMake-hdf5-1.12.1.tar.gz
        URL_MD5     2191996e136130c57b3508796ce32c2a
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        PATCH_COMMAND patch -p1 -t -N < ${CMAKE_CURRENT_LIST_DIR}/hdf5-1.12.1.patch
        BUILD_ALWAYS TRUE
        SOURCE_SUBDIR hdf5-1.12.1
        DEPENDS ${hdf5_depends}
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
        -DHDF5_PACKAGE_EXTLIBS:BOOL=ON
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DHDF5_ENABLE_COVERAGE:BOOL=OFF
        -DHDF5_ENABLE_TRACE:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=${HDF5_ENABLE_Z_LIB_SUPPORT}
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=${HDF5_ENABLE_SZIP_SUPPORT}
        -DHDF5_ENABLE_SZIP_ENCODING:BOOL=${HDF5_ENABLE_SZIP_ENCODING}
        -DZLIB_ROOT:PATH=${CMAKE_INSTALL_PREFIX}
        -DSZIP_ROOT:PATH=${CMAKE_INSTALL_PREFIX}
        -DLIBAEC_ROOT:PATH=${CMAKE_INSTALL_PREFIX}
        -DZLIB_INCLUDE_DIR:PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
        -DSZIP_INCLUDE_DIR:PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR} # Fixes bug when using libaec
        -DZLIB_PACKAGE_NAME:STRING=zlib
        -DSZIP_PACKAGE_NAME:STRING=libaec
        -DSZ_PACKAGE_NAME:STRING=libaec
        -DUSE_LIBAEC:BOOL=ON
        -DZLIB_USE_EXTERNAL:BOOL=OFF
        -DSZIP_USE_EXTERNAL:BOOL=OFF
        -DLIBAEC_USE_EXTERNAL:BOOL=OFF
        -DHDF5_BUILD_TOOLS:BOOL=${HDF5_BUILD_TOOLS}
        -DHDF5_BUILD_HL_LIB:BOOL=${HDF5_BUILD_HL_LIB}
        -DHDF5_BUILD_CPP_LIB:BOOL=${HDF5_BUILD_CPP_LIB}
        -DHDF5_BUILD_FORTRAN:BOOL=${HDF5_BUILD_FORTRAN}
        -DHDF5_BUILD_JAVA:BOOL=${HDF5_BUILD_JAVA}
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_DISABLE_COMPILER_WARNINGS:BOOL=ON
        -DALLOW_UNSUPPORTED=ON
        )

