
# This script attempts to find HDF5 installed from apt or from sources
# In both cases, if HDF5 is found a target "hdf5" is generated so the user can simply do:
#
#  target_link_libraries(mylibrary INTERFACE hdf5)
#
#
#  The user can guide the find pattern with variables:
#       BUILD_SHARED_LIBS           ON/OFF for shared/static libs
#       HDF5_REQUIRED               to require HDF5 to be found, set to ON
#       HDF5_WANT_VERSION           sets the required version (default 1.10)
#



find_file(HDF5_C_COMPILER_EXECUTABLE        NAMES h5cc  PATHS $ENV{PATH} /usr/bin /usr/local/bin $ENV{HOME}/.conda/bin  $ENV{HOME}/anaconda3/bin)
find_file(HDF5_CXX_COMPILER_EXECUTABLE      NAMES h5c++ PATHS $ENV{PATH} /usr/bin /usr/local/bin $ENV{HOME}/.conda/bin  $ENV{HOME}/anaconda3/bin)
if(BUILD_SHARED_LIBS)
    set(HDF5_TARGET_SUFFIX "shared")
    set(HDF5_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(HDF5_USE_STATIC_LIBRARIES OFF)
else()
    set(HDF5_TARGET_SUFFIX "static")
    set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(HDF5_USE_STATIC_LIBRARIES TRUE)
endif()
set(HDF5_FIND_DEBUG OFF)
if(NOT HDF5_WANT_VERSION)
    set(HDF5_WANT_VERSION 1.10)
endif()

if(HDF5_REQUIRED)
    find_package(HDF5 ${HDF5_WANT_VERSION} COMPONENTS C HL REQUIRED)
else()
    find_package(HDF5 ${HDF5_WANT_VERSION} COMPONENTS C HL)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

# To print all variables, use the code below:
##
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5" OR "${_variableName}" MATCHES "h5")
#        message(STATUS "${_variableName}=${${_variableName}}")
#    endif()
#endforeach()


if(HDF5_FOUND)
    # Add convenience libraries to collect all the hdf5 libraries
    add_library(hdf5    INTERFACE)
    add_library(hdf5::hdf5 ALIAS hdf5)
    if(TARGET hdf5::hdf5-${HDF5_TARGET_SUFFIX})
        set(HDF5_DIR              ${HDF5_BUILD_DIR}/share/cmake/hdf5)
        set(HDF5_ROOT             ${HDF5_BUILD_DIR})

        target_link_libraries(hdf5
                INTERFACE
                ${HDF5_C_HL_LIBRARY}
                ${HDF5_C_LIBRARY}
                $<LINK_ONLY:-ldl -lm>
                Threads::Threads
                )
        if(HDF5_ENABLE_Z_LIB_SUPPORT)
            target_link_libraries(hdf5 INTERFACE $<LINK_ONLY:-lz>  )
        endif()
        target_include_directories(hdf5
            INTERFACE
            ${HDF5_INCLUDE_DIR}
            )

    else()
#        add_dependencies(hdf5  SZIP)
        if (_HDF5_LPATH)
            set(HDF5_ROOT ${_HDF5_LPATH})
        endif()

        target_link_libraries(hdf5
                INTERFACE
                ${HDF5_C_LIBRARY_hdf5_hl}
                ${HDF5_C_LIBRARY_hdf5}
                $<LINK_ONLY:-ldl -lm>
                Threads::Threads
                )
        if(HDF5_C_LIBRARY_sz)
            target_link_libraries(hdf5 INTERFACE $<LINK_ONLY:-lsz>  )
        endif()
        if(HDF5_C_LIBRARY_z)
            target_link_libraries(hdf5 INTERFACE $<LINK_ONLY:-lz>  )
        endif()
        target_include_directories(
                hdf5
                INTERFACE
                ${HDF5_INCLUDE_DIR}
        )
    endif()
endif()
