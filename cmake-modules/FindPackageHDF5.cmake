
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



function(find_package_hdf5_isolator hdf5_root)
    unset(HDF5_CXX_COMPILER_EXECUTABLE CACHE)
    unset(HDF5_C_COMPILER_EXECUTABLE   CACHE)
    unset(HDF5_CXX_COMPILER_EXECUTABLE)
    unset(HDF5_C_COMPILER_EXECUTABLE  )
    unset(HDF5_FOUND )
    unset(HDF5_FOUND CACHE)
    unset(HDF5_FOUND PARENT_SCOPE)
    set(HDF5_FOUND False)
    set(HDF5_FIND_DEBUG OFF)
    set(HDF5_FIND_VERBOSE OFF)
    if(HDF5_FIND_VERBOSE)
        message(STATUS "Searching for hdf5 execs in ${hdf5_root}" )
    endif()
    set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE ON)
    find_file(HDF5_C_COMPILER_EXECUTABLE    NAMES h5cc  PATHS ${hdf5_root} PATH_SUFFIXES bin hdf5/bin NO_DEFAULT_PATH)
    find_file(HDF5_CXX_COMPILER_EXECUTABLE  NAMES h5c++ PATHS ${hdf5_root} PATH_SUFFIXES bin hdf5/bin NO_DEFAULT_PATH)
    if (HDF5_C_COMPILER_EXECUTABLE OR HDF5_CXX_COMPILER_EXECUTABLE)
        if(HDF5_FIND_VERBOSE)
            message(STATUS "Searching for hdf5 execs in ${hdf5_root} - Success -- C:  ${HDF5_C_COMPILER_EXECUTABLE}  CXX: ${HDF5_CXX_COMPILER_EXECUTABLE}" )
        endif()
        if("C" IN_LIST HDF5_MODULES)
            enable_language(C)
        endif()
        find_package(HDF5 ${HDF5_ATLEAST_VERSION} COMPONENTS  ${HDF5_MODULES})
    endif()
    if(HDF5_FOUND)
        # Add C_HL and CXX_HL to the language bindings
        set(HDF5_LANG)
        if("HL" IN_LIST HDF5_MODULES)
            list(APPEND HDF5_LANG HL)
            if("C" IN_LIST HDF5_LANGUAGE_BINDINGS)
                list(APPEND HDF5_LANG C_HL)
            endif()
            if("CXX" IN_LIST HDF5_LANGUAGE_BINDINGS)
                list(APPEND HDF5_LANG CXX_HL)
            endif()
            if("FORTRAN" IN_LIST HDF5_LANGUAGE_BINDINGS)
                list(APPEND HDF5_LANG FORTRAN_HL)
            endif()
        endif()
        foreach(lang ${HDF5_LANGUAGE_BINDINGS})
            list(APPEND HDF5_LANG ${lang} )
        endforeach()

        set(ACCEPT_PACKAGE TRUE)
        set(HDF5_LIBNAMES)
        set(HDF5_LINK_LIBNAMES)
        # Get a list of library names like hdf5 hdf5_hl hdf5_hl_cpp etc
        foreach(lang ${HDF5_LANG})
#            message(lang: ${lang})
            foreach(lib ${HDF5_${lang}_LIBRARY_NAMES})
#                message("INVESTIGATING: HDF5_${lang}_LIBRARY_${lib}: ${HDF5_${lang}_LIBRARY_${lib}} due to HDF5_${lang}_LIBRARY_NAMES : ${HDF5_${lang}_LIBRARY_NAMES}")
                if(${lib} MATCHES "hdf5")
                    list(APPEND HDF5_LIBNAMES ${lib})
                elseif(NOT ${lib} MATCHES "pthread")
                    list(APPEND HDF5_LINK_LIBNAMES ${lib})
                endif()
            endforeach()
        endforeach()
        list(REMOVE_DUPLICATES HDF5_LIBNAMES)
        # Check that each library has correct static/shared extension
        foreach(lang ${HDF5_LANG} )
            foreach(lib ${HDF5_LIBNAMES})
                get_filename_component(lib_extension "${HDF5_${lang}_LIBRARY_${lib}}" EXT)
                if(NOT ${lib_extension} MATCHES "${HDF5_LIBRARY_SUFFIX}" )
                    set(ACCEPT_PACKAGE FALSE)
                endif()
            endforeach()
        endforeach()

        if (NOT ACCEPT_PACKAGE)
            message(STATUS "HDF5 Package got disqualified")
        else()
            get_filename_component(HDF5_ROOT "${HDF5_CXX_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            if(NOT HDF5_ROOT)
                get_filename_component(HDF5_ROOT "${HDF5_C_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            endif()
            # To print all variables, use the code below:
            #  get_cmake_property(_variableNames VARIABLES)
            #  foreach (_variableName ${_variableNames})
            #      if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5")
            #          message(STATUS "${_variableName}=${${_variableName}}")
            #      endif()
            #  endforeach()

            set(HDF5_FOUND                  ${HDF5_FOUND}               PARENT_SCOPE)
            set(HDF5_ROOT                   ${HDF5_ROOT}                PARENT_SCOPE)
            set(HDF5_DIR                    ${HDF5_DIR}                 PARENT_SCOPE)
            set(HDF5_VERSION                ${HDF5_VERSION}             PARENT_SCOPE)
            set(HDF5_IS_PARALLEL            ${HDF5_IS_PARALLEL}         PARENT_SCOPE)

            set(HDF5_INCLUDE_DIR            ${HDF5_INCLUDE_DIR}         PARENT_SCOPE)
            set(HDF5_LIBRARIES "")
            set(HDF5_LINK_LIBRARY_NAMES "")
            foreach(lang ${HDF5_LANG})
                foreach(lib ${HDF5_LIBNAMES})
                    if(${lib} MATCHES "hdf5")
                        list(APPEND HDF5_LIBRARIES ${HDF5_${lang}_LIBRARY_${lib}})
                    endif()
                endforeach()
                foreach(lib ${HDF5_LINK_LIBNAMES})
                    if(NOT ${lib} MATCHES "pthread")
                        list(APPEND HDF5_LINK_LIBRARY_NAMES -l${lib})
                    endif()
                endforeach()
            endforeach()
            list(REMOVE_DUPLICATES HDF5_LIBRARIES)
            list(REMOVE_DUPLICATES HDF5_LINK_LIBRARY_NAMES)

            set(HDF5_LIBRARIES          ${HDF5_LIBRARIES}           PARENT_SCOPE)
            set(HDF5_LINK_LIBRARY_NAMES ${HDF5_LINK_LIBRARY_NAMES}  PARENT_SCOPE)
        endif()
    endif()
endfunction()




function(find_package_hdf5_internal hdf5_paths HDF5_MODULES HDF5_ATLEAST_VERSION HDF5_USE_STATIC_LIBRARIES HDF5_PREFER_PARALLEL HDF5_REQUIRED )

    foreach(hdf5_root ${hdf5_paths})
        find_package_hdf5_isolator("${hdf5_root}")
        if(HDF5_FOUND)
            set(HDF5_LIBRARIES           ${HDF5_LIBRARIES}              PARENT_SCOPE)
            set(HDF5_INCLUDE_DIR         ${HDF5_INCLUDE_DIR}            PARENT_SCOPE)
            set(HDF5_LINK_LIBRARY_NAMES  ${HDF5_LINK_LIBRARY_NAMES}     PARENT_SCOPE)
            set(HDF5_FOUND               ${HDF5_FOUND}                  PARENT_SCOPE)
            set(HDF5_ROOT                ${HDF5_ROOT}                   PARENT_SCOPE)
            set(HDF5_DIR                 ${HDF5_DIR}                    PARENT_SCOPE)
            set(HDF5_VERSION             ${HDF5_VERSION}                PARENT_SCOPE)
            set(HDF5_IS_PARALLEL         ${HDF5_IS_PARALLEL}            PARENT_SCOPE)
            return()
        else()
            set(HDF5_FOUND FALSE PARENT_SCOPE)
        endif()
    endforeach()
    if(HDF5_REQUIRED)
        message(FATAL_ERROR "Could not find HDF5 package")
    endif()
endfunction()

function(find_package_hdf5)
    if(NOT HDF5_MODULES)
        set(HDF5_MODULES C HL)
    endif()


    if(BUILD_SHARED_LIBS)
        set(HDF5_USE_STATIC_LIBRARIES OFF)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(HDF5_USE_STATIC_LIBRARIES ON)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()

    if(NOT HDF5_ATLEAST_VERSION)
        set(HDF5_ATLEAST_VERSION 1.8)
#        set(HDF5_ATLEAST_VERSION 1.10)
    endif()

    if(NOT HDF5_PREFER_PARALLEL)
        set(HDF5_PREFER_PARALLEL OFF)
    endif()

    if (NOT HDF5_REQUIRED)
        set(HDF5_REQUIRED OFF)
    endif()

    list(APPEND HDF5_PATHS
            ${hdf5_DIR}
            ${HDF5_DIR}
            $ENV{hdf5_DIR}
            $ENV{HDF5_DIR}
            ${H5PP_DIRECTORY_HINTS}
            ${HDF5_ROOT}
            $ENV{HDF5_ROOT}
            $ENV{EBROOTHDF5}
            /usr /usr/local
            ${CMAKE_INSTALL_PREFIX}
            $ENV{CONDA_PREFIX})
    find_package_hdf5_internal("${HDF5_PATHS}" "${HDF5_MODULES}" "${HDF5_ATLEAST_VERSION}" "${HDF5_USE_STATIC_LIBRARIES}" "${HDF5_PREFER_PARALLEL}" "${HDF5_REQUIRED}")
    # To print all variables, use the code below:
#    get_cmake_property(_variableNames VARIABLES)
#    foreach (_variableName ${_variableNames})
#        if("${_variableName}" MATCHES "HDF5" OR "${_variableName}" MATCHES "hdf5")
#            message(STATUS "${_variableName}=${${_variableName}}")
#        endif()
#    endforeach()
    if(HDF5_FOUND)

        # Add convenience libraries to collect all the hdf5 libraries
        add_library(hdf5::hdf5 IMPORTED INTERFACE GLOBAL)
        target_include_directories(hdf5::hdf5 INTERFACE  ${HDF5_INCLUDE_DIR})
        target_link_libraries(hdf5::hdf5
                INTERFACE
                ${HDF5_LIBRARIES}
                ${HDF5_LINK_LIBRARY_NAMES}
                )

        if("-lsz" IN_LIST HDF5_LINK_LIBRARY_NAMES)
            if (NOT BUILD_SHARED_LIBS)
                target_link_libraries(hdf5::hdf5 INTERFACE -laec)
            endif()
        endif()
        if(NOT TARGET Threads::Threads)
            ##################################################################
            ### Adapt pthread for static/dynamic linking                   ###
            ##################################################################
            set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
            set(THREADS_PREFER_PTHREAD_FLAG FALSE)
            find_package(Threads)
            if(TARGET Threads::Threads)
                if(NOT BUILD_SHARED_LIBS)
                    set_target_properties(Threads::Threads PROPERTIES INTERFACE_LINK_LIBRARIES "-Wl,--whole-archive -lpthread -Wl,--no-whole-archive")
                endif()
            endif()
        endif()

        if(TARGET Threads::Threads)
            target_link_libraries(hdf5::hdf5 INTERFACE  Threads::Threads)
        endif()



    endif()
endfunction()