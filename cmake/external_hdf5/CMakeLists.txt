cmake_minimum_required(VERSION 3.10)
project(external-eigen3)
include(../GetNumThreads.cmake)
get_num_threads(NUM_THREADS)
set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${NUM_THREADS})
include(ExternalProject)
option(HDF5_ENABLE_PARALLEL "" OFF)
option(HDF5_EXTERNALLY_CONFIGURED "" ON)
ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.5/src/hdf5-1.10.5.tar.gz
        URL_MD5     e115eeb66e944fa7814482415dd21cc4
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS OFF
#        CONFIGURE_COMMAND  pwd && CFLAGS=-fPIC && sh <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-cxx --enable-hl --enable-silent-rules
#        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -j ${NUM_THREADS}
#        INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install

        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_STATIC_EXECS:BOOL=OFF
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DBUILD_TESTING:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=OFF
        -DHDF5_BUILD_TOOLS:BOOL=ON
        -DHDF5_BUILD_FORTRAN:BOOL=OFF
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_BUILD_JAVA:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON
#        -DCMAKE_C_FLAGS=-w
        -DALLOW_UNSUPPORTED=ON
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console

#        -DHDF5_EXTERNALLY_CONFIGURED=1
#        -DHDF5_USE_PREGEN:BOOL=ON
#        -DHDF5_BATCH_H5DETECT:BOOL=ON
#        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        )
