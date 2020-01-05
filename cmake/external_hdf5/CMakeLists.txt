cmake_minimum_required(VERSION 3.10)
project(external-eigen3)
include(../GetNumThreads.cmake)
get_num_threads(NUM_THREADS)
set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${NUM_THREADS})
include(ExternalProject)
option(HDF5_ENABLE_PARALLEL "" OFF)
option(HDF5_EXTERNALLY_CONFIGURED "" ON)
option(HDF5_ENABLE_Z_LIB_SUPPORT "" ON)


if(HDF5_ENABLE_Z_LIB_SUPPORT)
    if(NOT BUILD_SHARED_LIBS)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
    find_package(ZLIB)
    if(TARGET ZLIB::ZLIB)
        get_target_property(ZLIB_LIBRARY ZLIB::ZLIB LOCATION)
        get_filename_component(ZLIB_EXT ${ZLIB_LIBRARY} EXT)
        if(NOT BUILD_SHARED_LIBS AND ZLIB_EXT MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}")
            message(STATUS "Found only shared zlib on static build: disabling zlib support for hdf5")
            set(HDF5_ENABLE_Z_LIB_SUPPORT OFF CACHE BOOL "Disabled zlib when not found" FORCE)
        endif()
    else()
        set(HDF5_ENABLE_Z_LIB_SUPPORT OFF CACHE BOOL "Disabled zlib when not found" FORCE)
    endif()
endif()


ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.6/src/hdf5-1.10.6.tar.gz
        URL_MD5     37f3089e7487daf0890baf3d3328e54a
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS OFF
#        CONFIGURE_COMMAND sh <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-cxx --enable-hl --enable-silent-rules
#        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -j ${NUM_THREADS}
#        INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install

        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_FIND_LIBRARY_SUFFIXES=${CMAKE_FIND_LIBRARY_SUFFIXES}
        -DBUILD_STATIC_EXECS:BOOL=ON
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DBUILD_TESTING:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=${HDF5_ENABLE_Z_LIB_SUPPORT}
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=OFF
        -DHDF5_BUILD_TOOLS:BOOL=ON
        -DHDF5_BUILD_FORTRAN:BOOL=OFF
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_BUILD_JAVA:BOOL=OFF
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON
        -DALLOW_UNSUPPORTED=ON
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console

#        -DHDF5_ALLOW_EXTERNAL_SUPPORT:STRING=GIT
#        -DZLIB_URL:STRING=https://github.com/madler/zlib.git
#        -DZLIB_BRANCH:STRING=develop

#        -DHDF5_EXTERNALLY_CONFIGURED=1
#        -DHDF5_USE_PREGEN:BOOL=ON
#        -DHDF5_BATCH_H5DETECT:BOOL=ON
#        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        )
