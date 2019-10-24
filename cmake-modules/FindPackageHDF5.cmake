
# This script attempts to find HDF5 installed from apt or from sources
# In both cases, if HDF5 is found a target "hdf5" is generated so the user can simply do:
#
#  target_link_libraries(mylibrary INTERFACE hdf5)
#
#
#  The user can guide the find pattern with variables:
#       HDF5_ROOT                   list of possible root directories to search in
#       HDF5_MODULES                list of modules, e.g, "C CXX HL"
#       BUILD_SHARED_LIBS           ON/OFF for shared/static libs
#       HDF5_REQUIRED               to require HDF5 to be found, set to ON
#       HDF5_ATLEAST_VERSION        sets the required version (default 1.10)
#







function(find_package_hdf5 hdf5_roots HDF5_MODULES HDF5_ATLEAST_VERSION HDF5_USE_STATIC_LIBRARIES HDF5_PREFER_PARALLEL HDF5_REQUIRED )

    foreach(hdf5_root ${hdf5_roots})
        message(STATUS "Searching for hdf5 execs in ${hdf5_root}" )
        unset(HDF5_CXX_COMPILER_EXECUTABLE CACHE)
        unset(HDF5_C_COMPILER_EXECUTABLE   CACHE)
        unset(HDF5_FOUND CACHE)
        unset(HDF5_FOUND PARENT_SCOPE)

        find_file(HDF5_C_COMPILER_EXECUTABLE    NAMES h5cc  PATH_SUFFIXES bin envs/bin dmrg/bin envs/dmrg/bin PATHS ${hdf5_root} )
        find_file(HDF5_CXX_COMPILER_EXECUTABLE  NAMES h5c++ PATH_SUFFIXES bin envs/bin dmrg/bin envs/dmrg/bin PATHS ${hdf5_root} )
        if (HDF5_C_COMPILER_EXECUTABLE OR HDF5_CXX_COMPILER_EXECUTABLE)
            message(STATUS "Searching for hdf5 execs in ${hdf5_root} - Success -- C:  ${HDF5_C_COMPILER_EXECUTABLE}  CXX: ${HDF5_CXX_COMPILER_EXECUTABLE}" )
            set(HDF5_FIND_DEBUG OFF)
            set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE ON)
            find_package(HDF5 ${HDF5_ATLEAST_VERSION} COMPONENTS ${HDF5_MODULES})

            #            get_cmake_property(_variableNames VARIABLES)
            #            foreach (_variableName ${_variableNames})
            #                if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5" OR "${_variableName}" MATCHES "h5")
            #                    message(STATUS "${_variableName}=${${_variableName}}")
            #                endif()
            #            endforeach()
            if(HDF5_FOUND)
                set(ACCEPT_PACKAGE FALSE)
                if(HDF5_PREFER_PARALLEL AND HDF5_IS_PARALLEL)
                    message(STATUS "Found parallel HDF5 package version: ${HDF5_VERSION}")
                    set(ACCEPT_PACKAGE TRUE)
                elseif(NOT HDF5_PREFER_PARALLEL AND NOT HDF5_IS_PARALLEL)
                    message(STATUS "Found serial HDF5 package version: ${HDF5_VERSION}")
                    set(ACCEPT_PACKAGE TRUE)
                else()
                    message(STATUS "HDF5 package found but not correctly serial/parallel")
                endif()

                if(ACCEPT_PACKAGE)
                    get_filename_component(hdf5_root "${HDF5_CXX_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
                    if(NOT hdf5_root)
                        get_filename_component(hdf5_root "${HDF5_C_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
                    endif()
                    set(HDF5_FOUND                  ${HDF5_FOUND}               PARENT_SCOPE)
                    set(HDF5_ROOT                   ${hdf5_root}                PARENT_SCOPE)
                    set(HDF5_DIR                    ${HDF5_DIR}                 PARENT_SCOPE)
                    set(HDF5_VERSION                ${HDF5_VERSION}             PARENT_SCOPE)
                    set(HDF5_IS_PARALLEL            ${HDF5_IS_PARALLEL}         PARENT_SCOPE)
                    set(HDF5_C_DEFINITIONS          ${HDF5_C_DEFINITIONS}       PARENT_SCOPE)
                    set(HDF5_hdf5_hl_LIBRARY        ${HDF5_hdf5_hl_LIBRARY}     PARENT_SCOPE)
                    set(HDF5_C_LIBRARY_hdf5_hl      ${HDF5_C_LIBRARY_hdf5_hl}   PARENT_SCOPE)
                    set(HDF5_C_HL_LIBRARY           ${HDF5_C_HL_LIBRARY}        PARENT_SCOPE)
                    set(HDF5_hdf5_LIBRARY           ${HDF5_hdf5_LIBRARY}        PARENT_SCOPE)
                    set(HDF5_C_LIBRARY              ${HDF5_C_LIBRARY}           PARENT_SCOPE)
                    set(HDF5_C_LIBRARY_hdf5         ${HDF5_C_LIBRARY_hdf5}      PARENT_SCOPE)

                    set(HDF5_C_LIBRARY_sz           ${HDF5_C_LIBRARY_sz}        PARENT_SCOPE)
                    set(HDF5_C_LIBRARY_m            ${HDF5_C_LIBRARY_m}         PARENT_SCOPE)
                    set(HDF5_C_LIBRARY_rt           ${HDF5_C_LIBRARY_rt}        PARENT_SCOPE)
                    set(HDF5_C_LIBRARY_z            ${HDF5_C_LIBRARY_z}         PARENT_SCOPE)
                    set(HDF5_C_LIBRARY_dl           ${HDF5_C_LIBRARY_dl}        PARENT_SCOPE)

                    set(HDF5_INCLUDE_DIR            ${HDF5_INCLUDE_DIR}         PARENT_SCOPE)
                    set(HDF5_INCLUDE_DIRS           ${HDF5_INCLUDE_DIRS}        PARENT_SCOPE)
                    set(HDF5_LIBRARIES
                            ${HDF5_hdf5_hl_LIBRARY}
                            ${HDF5_C_LIBRARY_hdf5_hl}
                            ${HDF5_C_HL_LIBRARY}
                            ${HDF5_hdf5_LIBRARY}
                            ${HDF5_C_LIBRARY}
                            ${HDF5_C_LIBRARY_hdf5}
                            PARENT_SCOPE)
                    return()
                endif()
            endif()
        else()
            message(STATUS "Searching for hdf5 execs in ${hdf5_root} - failed:" )
            set(HDF5_FOUND FALSE PARENT_SCOPE)
            #            return()
        endif()
    endforeach()
    if(HDF5_REQUIRED)
        message(FATAL_ERROR "Could not find HDF5 package")
    endif()
endfunction()


if(NOT HDF5_MODULES)
    set(HDF5_MODULES C CXX HL)
endif()


if(BUILD_SHARED_LIBS)
    set(HDF5_USE_STATIC_LIBRARIES OFF)
    set(HDF5_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(HDF5_USE_STATIC_LIBRARIES ON)
    set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

if(NOT HDF5_ATLEAST_VERSION)
    set(HDF5_ATLEAST_VERSION 1.10)
endif()

if(NOT HDF5_PREFER_PARALLEL)
    set(HDF5_PREFER_PARALLEL OFF)
endif()

if (NOT HDF5_REQUIRED)
    set(HDF5_REQUIRED OFF)
endif()

set(HDF5_ROOT ${HDF5_ROOT} ${HDF5_DIR} $ENV{HDF5_ROOT} $ENV{HDF5_DIR} $ENV{EBROOTHDF5} $ENV{HOME}/.conda $ENV{HOME}/anaconda3 $ENV{HOME}/miniconda3 /usr /usr/local)

find_package_hdf5("${HDF5_ROOT}" "${HDF5_MODULES}" "${HDF5_ATLEAST_VERSION}" "${HDF5_USE_STATIC_LIBRARIES}" "${HDF5_PREFER_PARALLEL}" "${HDF5_REQUIRED}")

# To print all variables, use the code below:
##
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    string( TOLOWER "${_variableName}" _variableName_low )
#    if("${_variableName_low}" MATCHES "hdf5" OR "${_variableName_low}" MATCHES "h5")
#        message(STATUS "${_variableName}=${${_variableName}}")
#    endif()
#endforeach()


if(HDF5_FOUND)
    # Add convenience libraries to collect all the hdf5 libraries
    add_library(hdf5    INTERFACE)
    add_library(hdf5::hdf5 ALIAS hdf5)
    target_link_libraries(hdf5
            INTERFACE
            ${HDF5_LIBRARIES}
            $<LINK_ONLY:-ldl -lm -lz>
            Threads::Threads
            )
    target_include_directories(hdf5 INTERFACE  ${HDF5_INCLUDE_DIR})
    if(HDF5_C_LIBRARY_sz)
        target_link_libraries(hdf5 INTERFACE $<LINK_ONLY:-lsz>)
        if (NOT BUILD_SHARED_LIBS)
            target_link_libraries(hdf5 INTERFACE $<LINK_ONLY: -laec>)
        endif()
    endif()

endif()
