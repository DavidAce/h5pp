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

if(HDF5_ENABLE_Z_LIB_SUPPORT)
    find_package(ZLIB REQUIRED)
    get_target_property(ZLIB_LIBRARY     ZLIB::ZLIB LOCATION )
    get_target_property(ZLIB_INCLUDE_DIR ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
    message(DEBUG "HDF5: Found ZLIB LIBRARY: ${ZLIB_LIBRARY}")
    message(DEBUG "HDF5: Found ZLIB INCLUDE: ${ZLIB_INCLUDE_DIR}")
endif()
if(HDF5_ENABLE_SZIP_SUPPORT)
    find_package(SZIP NAMES szip COMPONENTS static shared REQUIRED)
    get_target_property(SZIP_LIBRARY     szip-static LOCATION )
    get_target_property(SZIP_INCLUDE_DIR szip-static INTERFACE_INCLUDE_DIRECTORIES)
    message(DEBUG "HDF5: Found SZIP LIBRARY: ${SZIP_LIBRARY}")
    message(DEBUG "HDF5: Found SZIP INCLUDE: ${SZIP_INCLUDE_DIR}")
endif()

ExternalProject_Add(external_hdf5
#        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.1/src/hdf5-1.12.1.tar.gz
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.13/hdf5-1.13.1/src/hdf5-1.13.1.tar.gz
#        URL_MD5     e4f057f0a3eb4237c7799605357a584d # 1.12.1
        URL_MD5     1daffd4662ba2207a96a93272244bb77  # 1.13.1
        PREFIX      ${CMAKE_BINARY_DIR}
        # We need a patch here to get HDF5 to export zlib and szip as public dependencies on both static and shared targets
        PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/hdf5-link-public.patch.cmake
        # We need another patch to force HDF5 to link ZLIB::ZLIB and szip-static targets rather than full paths to libraries
        PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/hdf5-link-target.patch.cmake
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        LIST_SEPARATOR |
        CMAKE_ARGS
        ${INIT_CACHE_ARG}
        -DINSTALLDIR=<INSTALL_DIR>
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
        -DHDF5_PACKAGE_EXTLIBS:BOOL=OFF # So that hdf5-config.cmake includes zlib-config.cmake and szip-config.cmake.
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DHDF5_ENABLE_COVERAGE:BOOL=OFF
        -DHDF5_ENABLE_TRACE:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=${HDF5_ENABLE_Z_LIB_SUPPORT}
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=${HDF5_ENABLE_SZIP_SUPPORT}
        -DHDF5_ENABLE_SZIP_ENCODING:BOOL=${HDF5_ENABLE_SZIP_ENCODING}
#        -DZLIB_FOUND:BOOL=TRUE
#        -DSZIP_FOUND:BOOL=TRUE
        -DZLIB_LIBRARY:STRING=${ZLIB_LIBRARY} # This is to force FindZLIB.cmake to find and define a target with the correct library
#        -DSZIP_LIBRARY:STRING=${SZIP_LIBRARY}
        -DSZIP_LIBRARY:STRING=szip-static
        -DZLIB_STATIC_LIBRARY:STRING=ZLIB::ZLIB  # This is to use target-based linking instead of hardcoding a path to file
        -DSZIP_STATIC_LIBRARY:STRING=szip-static # This is to use target-based linking instead of hardcoding a path to file
        -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
        -DSZIP_INCLUDE_DIR:PATH=${SZIP_INCLUDE_DIR}
#        -DZLIB_ROOT:PATH=${ZLIB_ROOT}
#        -DSZIP_ROOT:PATH=${SZIP_ROOT}
#        -DZLIB_DIR:PATH=${ZLIB_ROOT}/share/cmake/zlib
#        -DSZIP_DIR:PATH=${SZIP_ROOT}/share/cmake/szip
#        -DZLIB_PACKAGE_NAME:STRING=zlib
#        -DSZIP_PACKAGE_NAME:STRING=szip
#        -DSZ_PACKAGE_NAME:STRING=szip
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

