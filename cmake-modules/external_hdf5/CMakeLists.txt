cmake_minimum_required(VERSION 3.10)
project(external-eigen3)

include(ExternalProject)
set(HDF5_IS_PARALLEL OFF)
set(HDF5_EXTERNALLY_CONFIGURED 1)
ExternalProject_Add(external_hdf5
        URL     https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.5/src/hdf5-1.10.5.tar.bz2
        PREFIX      ${BUILD_DIRECTORY}/hdf5
        INSTALL_DIR ${INSTALL_DIRECTORY}/hdf5
#        UPDATE_DISCONNECTED 1
        BUILD_ALWAYS 1
#        BINARY_DIR ${BUILD_DIR}/hdf5
        CONFIGURE_COMMAND  pwd && CFLAGS=-fPIC && sh <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-cxx --enable-hl --enable-silent-rules
        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -j
        INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install

#        CMAKE_ARGS
#        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
#        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
#        -DBUILD_STATIC_EXECS:BOOL=OFF
#        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
#        -DBUILD_TESTING:BOOL=OFF
#        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=OFF
#        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=OFF
#        -DHDF5_BUILD_TOOLS:BOOL=ON
#        -DHDF5_BUILD_FORTRAN:BOOL=OFF
#        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
#        -DHDF5_BUILD_JAVA:BOOL=OFF
#        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON
#        -DCMAKE_C_FLAGS=-w -fPIC

        #        -DHDF5_EXTERNALLY_CONFIGURED=1
#        -DHDF5_USE_PREGEN:BOOL=ON
#        -DHDF5_BATCH_H5DETECT:BOOL=ON
#        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
#        -DALLOW_UNSUPPORTED=ON
#        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
        )
