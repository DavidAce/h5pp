
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



find_file(HDF5_C_COMPILER_EXECUTABLE        NAMES h5cc  HINTS /usr/bin /usr/local/bin )
find_file(HDF5_CXX_COMPILER_EXECUTABLE      NAMES h5c++ HINTS /usr/bin /usr/local/bin )
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

    find_package(HDF5 ${HDF5_WANT_VERSION} COMPONENTS C CXX HL REQUIRED)
else()
    find_package(HDF5 ${HDF5_WANT_VERSION} COMPONENTS C CXX HL)
endif()



if(HDF5_FOUND)
    # Add convenience libraries to collect all the hdf5 libraries
    add_library(hdf5    INTERFACE)
    if(TARGET hdf5::hdf5-${HDF5_TARGET_SUFFIX})
        get_cmake_property(_variableNames VARIABLES)
#        foreach (_variableName ${_variableNames})
#            if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5" OR "${_variableName}" MATCHES "h5")
#                message(STATUS "${_variableName}=${${_variableName}}")
#            endif()
#        endforeach()
        set(HDF5_DIR              ${HDF5_BUILD_DIR}/share/cmake/hdf5)
        set(HDF5_ROOT             ${HDF5_BUILD_DIR})


        message(STATUS "HDF5 FOUND PRE-INSTALLED: ${HDF5_BUILD_DIR}")
        target_link_libraries(hdf5
                INTERFACE
                hdf5::hdf5-${HDF5_TARGET_SUFFIX}
                hdf5::hdf5_hl-${HDF5_TARGET_SUFFIX}
                hdf5::hdf5_cpp-${HDF5_TARGET_SUFFIX}
                hdf5::hdf5_hl_cpp-${HDF5_TARGET_SUFFIX}
                )



    else()
#            get_cmake_property(_variableNames VARIABLES)
#            foreach (_variableName ${_variableNames})
#                if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5" OR "${_variableName}" MATCHES "h5")
#                    message(STATUS "${_variableName}=${${_variableName}}")
#                endif()
#            endforeach()
        message(STATUS "HDF5 FOUND IN SYSTEM: ${HDF5_LIBRARIES}")
        add_dependencies(hdf5  SZIP)
        if (_HDF5_LPATH AND NOT HDF5_ROOT)
            set(HDF5_ROOT ${_HDF5_LPATH})
        endif()


        target_link_libraries(
            hdf5
            INTERFACE
            ${HDF5_HL_LIBRARIES}
            ${HDF5_LIBRARIES}
#            ${HDF5_CXX_HL_LIBRARIES}
#            ${HDF5_CXX_LIBRARY_hdf5}
#            ${HDF5_CXX_LIBRARY_hdf5_hl}
#            ${HDF5_CXX_LIBRARY_hdf5_cpp}
#            ${HDF5_CXX_LIBRARY_hdf5_hl_cpp}
#            ${HDF5_CXX_LIBRARY_iomp5} ${HDF5_CXX_LIBRARY_sz}
#            $<LINK_ONLY:${HDF5_CXX_LIBRARY_pthread}>
#            $<LINK_ONLY:"-Wl,--no-as-needed -ldl -lm -lz -Wl,--as-needed">

        )

        target_include_directories(
                hdf5
                INTERFACE
                ${HDF5_INCLUDE_DIR}
        )





    endif()
endif()