cmake_minimum_required(VERSION 3.10)
project(external-eigen3)

include(ExternalProject)
set(HDF5_IS_PARALLEL OFF)
ExternalProject_Add(external_hdf5
        URL     https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.3/src/hdf5-1.10.3.tar.bz2
        PREFIX      ${BUILD_DIR}/hdf5
        INSTALL_DIR ${INSTALL_DIR}/hdf5
        UPDATE_DISCONNECTED 1
        TEST_COMMAND ""
        BUILD_ALWAYS 1
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_ANSI_CFLAGS:STRING=-fPIC
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=OFF
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DALLOW_UNSUPPORTED=ON
        -DBUILD_TESTING:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
        -DHDF5_BUILD_TOOLS:BOOL=ON
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_BUILD_FORTRAN:BOOL=OFF
        -DHDF5_BUILD_JAVA:BOOL=OFF
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
        -DCMAKE_C_FLAGS=-w
        )
