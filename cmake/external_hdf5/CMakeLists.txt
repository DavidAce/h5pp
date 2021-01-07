cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0074 NEW)
project(external-hdf5 C CXX)

include(ExternalProject)
option(HDF5_ENABLE_PARALLEL "Enables HDF5 Parallel MPI" OFF)
option(HDF5_EXTERNALLY_CONFIGURED "" ON)

if(ENV{CONDA_PREFIX})
    message(WARNING "The current conda environment may conflict with the build of HDF5: $ENV{CONDA_PREFIX}")
endif()

if(BUILD_SHARED_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()


# The following check is needed because HDF5 will blindly use
# find_package(ZLIB), which finds the shared library
# (even when a static is present) and use it to link to static
# hdf5 libraries, causing a build error. Even worse, if the build
# succeeds it will hardcode the path to libz.so for as an interface
# library for the static library.
# Here we circumvent that by specifying "z" as the zlib library
# so the hdf5-static target gets $<LINK_ONLY:z> ---> "-lz" as link
# flag instead. Of course this only works if the shared/static library
# actualy exists.

if(NOT HDF5_ENABLE_Z_LIB_SUPPORT OR NOT ZLIB_LIBRARY)

    set(HDF5_ENABLE_Z_LIB_SUPPORT ON)
    find_package(ZLIB)
    unset(ZLIB_LIBRARY)
    if(TARGET ZLIB::ZLIB)
        get_target_property(ZLIB_LIBRARY ZLIB::ZLIB LOCATION)
        get_filename_component(ZLIB_EXT ${ZLIB_LIBRARY} EXT)
        get_filename_component(ZLIB_WE  ${ZLIB_LIBRARY} NAME_WE)
        get_filename_component(ZLIB_DIR  ${ZLIB_LIBRARY} DIRECTORY)
        if(NOT ZLIB_EXT MATCHES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
            message(STATUS "find_package(ZLIB) found library ${ZLIB_LIBRARY} which has incorrect suffix. Attempting fix")
            find_library(ZLIB_LIBRARY NAMES ${ZLIB_WE}${CMAKE_FIND_LIBRARY_SUFFIXES} libz${CMAKE_FIND_LIBRARY_SUFFIXES} HINTS ${ZLIB_DIR})
            if(ZLIB_LIBRARY)
                set(ZLIB_LIBRARY z) # Replace the full path so that the linker can decide later instead
                set(HDF5_ENABLE_Z_LIB_SUPPORT ON)
            else()
                unset(ZLIB_LIBRARY)
                message(STATUS "Could not find static ZLIB: disabling ZLIB support for hdf5")
                set(HDF5_ENABLE_Z_LIB_SUPPORT OFF)
            endif()
        else()
            set(ZLIB_LIBRARY z) # Replace the full path so that the linker can decide later instead
            message(STATUS "find_package(ZLIB) found library ${ZLIB_LIBRARY} which has the correct suffix")
        endif()
    else()
        message(STATUS "Could not find ZLIB: disabling ZLIB support for hdf5")
        set(HDF5_ENABLE_Z_LIB_SUPPORT OFF)
    endif()
endif()



# The following check is needed because HDF5 will blindly use
# find_package(SZIP), which finds the shared library
# (even when a static is present) and use it to link to static
# hdf5 libraries, causing a build error. Even worse, if the build
# succeeds it will hardcode the path to libsz.so for as an interface
# library for the static library.
# Here we circumvent that by specifying "sz" as the szip library
# so the hdf5-static target gets $<LINK_ONLY:sz> ---> "-lsz" as link
# flag instead. Of course this only works if the shared/static library
# actualy exists.

# The following check is needed because HDF5 will blindly use
# find_package(SZIP), which finds the shared library
# (even when a static is present) and use it to link to static
# hdf5 libraries, causing a build error. Here we circumvent that
# by specifying the shared/static SZIP library explicitly as needed
if(NOT HDF5_ENABLE_SZIP_SUPPORT OR NOT SZIP_LIBRARY)
    set(HDF5_ENABLE_SZIP_SUPPORT ON)
    find_library(SZIP_LIBRARY NAMES sz) # No built in FindSZIP.cmake
    if(SZIP_LIBRARY)
        message(STATUS "Found SZIP: ${SZIP_LIBRARY}")
        get_filename_component(SZIP_EXT ${SZIP_LIBRARY} EXT)
        if(SZIP_EXT MATCHES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
            set(SZIP_LIBRARY sz)
            find_library(AEC_LIBRARY NAMES aec)
            if(AEC_LIBRARY)
                message(STATUS "Found AEC: ${AEC_LIBRARY}")
                set(SZIP_LIBRARY "sz$<SEMICOLON>aec")
            endif()
        else()
            message(STATUS "Could not find SZIP with the correct extension: disabling SZIP support for HDF5")
            set(HDF5_ENABLE_SZIP_SUPPORT OFF)
            unset(SZIP_LIBRARY)
        endif()
    else()
        message(STATUS "Could not find SZIP: disabling SZIP support for hdf5")
        set(HDF5_ENABLE_SZIP_SUPPORT OFF)
        unset(SZIP_LIBRARY)
    endif()
endif()

ExternalProject_Add(external_hdf5
        URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0.tar.gz
        URL_MD5     9e22217d22eb568e09f0cc15fb641d7c
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
        # CMake flags
        -DCMAKE_POLICY_DEFAULT_CMP0074=${CMAKE_POLICY_DEFAULT_CMP0074}
        -DCMAKE_EXE_LINKER_FLAGS_INIT=${CMAKE_EXE_LINKER_FLAGS}
        -DCMAKE_SHARED_LINKER_FLAGS_INIT=${CMAKE_SHARED_LINKER_FLAGS}
        -DCMAKE_STATIC_LINKER_FLAGS_INIT=${CMAKE_STATIC_LINKER_FLAGS}
        -DCMAKE_MODULE_LINKER_FLAGS_INIT=${CMAKE_MODULE_LINKER_FLAGS}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=${CMAKE_POSITION_INDEPENDENT_CODE}
        -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
        -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
        -DCMAKE_CXX_FLAGS_INIT:STRING=${CMAKE_CXX_FLAGS}
        -DCMAKE_CXX_FLAGS_RELEASE_INIT:STRING=${CMAKE_CXX_FLAGS_RELEASE}
        -DCMAKE_CXX_FLAGS_DEBUG_INIT:STRING=${CMAKE_CXX_FLAGS_DEBUG}
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
        -DCMAKE_CXX_FLAGS_MINSIZEREL_INIT:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        # HDF5 flags
#        -DBUILD_STATIC_EXECS:BOOL=OFF
#        -DBUILD_STATIC_LIBS:BOOL=ON
        -DBUILD_TESTING:BOOL=OFF
#        -DZLIB_LIBRARY:PATH=${ZLIB_LIBRARY}
        -DZLIB_LIBRARY=${ZLIB_LIBRARY}
        -DSZIP_LIBRARY=${SZIP_LIBRARY}
        -DHDF5_ENABLE_PARALLEL:BOOL=${HDF5_ENABLE_PARALLEL}
        -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=${HDF5_ENABLE_Z_LIB_SUPPORT}
        -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=${HDF5_ENABLE_SZIP_SUPPORT}
        -DHDF5_BUILD_TOOLS:BOOL=ON
        -DHDF5_BUILD_FORTRAN:BOOL=OFF
        -DHDF5_BUILD_EXAMPLES:BOOL=OFF
        -DHDF5_BUILD_JAVA:BOOL=OFF
        -DHDF5_DISABLE_COMPILER_WARNINGS:BOOL=ON
        -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON
        -DALLOW_UNSUPPORTED=ON
        )

