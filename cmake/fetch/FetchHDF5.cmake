cmake_minimum_required(VERSION 3.14)


option(HDF5_ENABLE_PARALLEL "Enables HDF5 Parallel MPI" OFF)

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
if(NOT HDF5_ENABLE_SZIP_SUPPORT OR NOT SZIP_LIBRARY AND NOT CMAKE_HOST_APPLE)
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

function(FetchHDF5)

    option(HDF5_GENERATE_HEADERS "Turn off header generation for HDF5" OFF)
    option(HDF5_DISABLE_COMPILER_WARNINGS "Turn off HDF5 compiler warnings" ON)

    option(BUILD_TESTING "" OFF)
    option(HDF5_NO_PACKAGES "" ON)
    option(HDF5_ENABLE_ANALYZER_TOOLS "" OFF)
    option(HDF5_BUILD_TOOLS "" OFF)
    option(HDF5_BUILD_FORTRAN "" OFF)
    option(HDF5_BUILD_EXAMPLES "" OFF)
    option(HDF5_BUILD_JAVA "" OFF)
    option(HDF5_DISABLE_COMPILER_WARNINGS "" ON)
    option(ALLOW_UNSUPPORTED "" ON)
    option(HDF5_EXTERNALLY_CONFIGURED "" OFF)
    option(HDF5_INSTALL_NO_DEVELOPMENT "" OFF)
    option(HDF5_PACKAGE_EXTLIBS "Create install package with external libraries (szip, zlib)" ON)
    include(FetchContent)
    FetchContent_Declare(fetch-hdf5
            URL         https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0.tar.gz
            URL_MD5     9e22217d22eb568e09f0cc15fb641d7c
    )

    FetchContent_MakeAvailable(fetch-hdf5)


    add_library(hdf5::all IMPORTED INTERFACE)
    target_Link_libraries(hdf5::all INTERFACE ${HDF5_LIBRARIES_TO_EXPORT})
endfunction()


FetchHDF5()
