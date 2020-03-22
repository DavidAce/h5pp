cmake_minimum_required(VERSION 3.12)
project(external-hdf5)

include(ExternalProject)
option(HDF5_ENABLE_PARALLEL "" OFF)
option(HDF5_EXTERNALLY_CONFIGURED "" ON)


# The following check is needed because HDF5 will blindly use
# find_package(ZLIB), which finds the shared library
# (even when a static is present) and use it to link to static
# hdf5 libraries, causing a build error. Here we circumvent that
# by specifying the shared/static zlib library explicitly as needed

set(HDF5_ENABLE_Z_LIB_SUPPORT ON)
if(BUILD_SHARED_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

find_package(ZLIB)
if(ZLIB_LIBRARY)
    get_filename_component(ZLIB_EXT ${ZLIB_LIBRARY} EXT)
    get_filename_component(ZLIB_WE  ${ZLIB_LIBRARY} NAME_WE)
    get_filename_component(ZLIB_DIR  ${ZLIB_LIBRARY} DIRECTORY)
    if(BUILD_SHARED_LIBS AND ZLIB_EXT MATCHES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
        if(EXISTS ${ZLIB_DIR}/${ZLIB_WE}${CMAKE_SHARED_LIBRARY_SUFFIX} )
            message(STATUS "Changing ZLIB extension ${ZLIB_EXT} -> ${CMAKE_SHARED_LIBRARY_SUFFIX}")
            set(ZLIB_LIBRARY ${ZLIB_DIR}/${ZLIB_WE}${CMAKE_SHARED_LIBRARY_SUFFIX} )
        else()
            message(STATUS "Could not find shared ZLIB: disabling ZLIB support for hdf5")
            set(HDF5_ENABLE_Z_LIB_SUPPORT OFF)
        endif()
    elseif(NOT BUILD_SHARED_LIBS AND ZLIB_EXT MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}")
        if(EXISTS ${ZLIB_DIR}/${ZLIB_WE}${CMAKE_STATIC_LIBRARY_SUFFIX} )
            message(STATUS "Changing ZLIB extension ${ZLIB_EXT} -> ${CMAKE_STATIC_LIBRARY_SUFFIX}")
            set(ZLIB_LIBRARY ${ZLIB_DIR}/${ZLIB_WE}${CMAKE_STATIC_LIBRARY_SUFFIX} )
        else()
            message(STATUS "Could not find static ZLIB: disabling ZLIB support for hdf5")
            set(HDF5_ENABLE_Z_LIB_SUPPORT OFF)
        endif()
    endif()
else()
    unset(ZLIB_LIBRARY)
    message(STATUS "Could not find ZLIB: disabling ZLIB support for hdf5")
    set(HDF5_ENABLE_Z_LIB_SUPPORT OFF)
endif()

# The following check is needed because HDF5 will blindly use
# find_package(SZIP), which finds the shared library
# (even when a static is present) and use it to link to static
# hdf5 libraries, causing a build error. Here we circumvent that
# by specifying the shared/static SZIP library explicitly as needed

#set(HDF5_ENABLE_SZIP_SUPPORT ON)
#set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
#find_library(SZIP_LIBRARY NAMES sz szip szip-static libsz libszip libszip-static) # No built in findSZIP.cmake
#if(SZIP_LIBRARY)
#    message(STATUS "Found SZIP: ${SZIP_LIBRARY}")
#    get_filename_component(SZIP_EXT ${SZIP_LIBRARY} EXT)
#    if(NOT BUILD_SHARED_LIBS AND SZIP_EXT MATCHES "${CMAKE_SHARED_LIBRARY_SUFFIX}")
#        message(STATUS "Could not find static SZIP: disabling SZIP support for hdf5")
#        set(HDF5_ENABLE_SZIP_SUPPORT OFF)
#    else()
#        find_library(AEC_LIBRARY NAMES aec)
#        if(AEC_LIBRARY)
#            message(STATUS "Found AEC: ${AEC_LIBRARY}")
#            list(APPEND CMAKE_SHARED_LINKER_FLAGS -laec)
#            list(APPEND SZIP_LIBRARY ${AEC_LIBRARY})
#        endif()
#    endif()
#
#else()
#    unset(SZIP_LIBRARY)
#    message(STATUS "Could not find SZIP: disabling SZIP support for hdf5")
#    set(HDF5_ENABLE_SZIP_SUPPORT OFF)
#endif()


ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0.tar.gz
        URL_MD5     9e22217d22eb568e09f0cc15fb641d7c
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS OFF

        CMAKE_ARGS
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DBUILD_TESTING:BOOL=OFF
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=${HDF5_ENABLE_Z_LIB_SUPPORT}
        -DZLIB_LIBRARY:PATH=${ZLIB_LIBRARY}
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=OFF # Complains about undefined reference to functions in libaec
        -DHDF5_BUILD_TOOLS:BOOL=ON
        -DHDF5_BUILD_FORTRAN:BOOL=OFF
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_BUILD_JAVA:BOOL=OFF
        -DHDF5_DISABLE_COMPILER_WARNINGS:BOOL=ON
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON
        -DALLOW_UNSUPPORTED=ON
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console

        #        -DHDF5_ALLOW_EXTERNAL_SUPPORT:STRING=GIT
        #        -DZLIB_URL:STRING=https://github.com/madler/zlib.git
        #        -DZLIB_BRANCH:STRING=develop
        #        -DSZIP_LIBRARY:PATH=${SZIP_LIBRARY}
        #        -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
        #        -DHDF5_EXTERNALLY_CONFIGURED=1
        #        -DHDF5_USE_PREGEN:BOOL=ON
        #        -DHDF5_BATCH_H5DETECT:BOOL=ON
)