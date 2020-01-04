
# This script attempts to find HDF5 installed from apt,conda or specified directories
# In all cases, if HDF5 is found a target "hdf5::hdf5" is generated so the user can simply do:
#
#  target_link_libraries(myExe INTERFACE hdf5::hdf5)
#
#
#  The user can guide the find pattern with variables:
#       HDF5_PATHS                  list of possible root directories to search in
#       HDF5_COMPONENTS             list of modules, e.g, "C CXX HL"
#       BUILD_SHARED_LIBS           ON/OFF for shared/static libs
#       HDF5_REQUIRED               to require HDF5 to be found, set to ON
#       HDF5_ATLEAST_VERSION        sets the required version (default 1.8)
#

function(define_hdf5_target lang libnames target_list)
#    message("lang: ${lang}  libnames: ${libnames}")
    list(GET libnames 0 lib)
    if(TARGET hdf5::${lib}_${HDF5_TARGET_SUFFIX})
        return()
    endif()

    list(LENGTH libnames numlibs)
    if(numlibs GREATER 1)
        list(SUBLIST libnames 1 -1 othernames)
    endif()
#    message("othernames: ${othernames}")

    if(HDF5_${lang}_LIBRARY_${lib})
        add_library(hdf5::${lib}_${HDF5_TARGET_SUFFIX} ${HDF5_LINK_TYPE} IMPORTED)
        set_target_properties(hdf5::${lib}_${HDF5_TARGET_SUFFIX} PROPERTIES IMPORTED_LOCATION  ${HDF5_${lang}_LIBRARY_${lib}})
    elseif(HDF5_C_LIBRARY_${lib})
        add_library(hdf5::${lib}_${HDF5_TARGET_SUFFIX} ${HDF5_LINK_TYPE} IMPORTED)
        set_target_properties(hdf5::${lib}_${HDF5_TARGET_SUFFIX} PROPERTIES IMPORTED_LOCATION  ${HDF5_C_LIBRARY_${lib}})
    elseif(HDF5_CXX_LIBRARY_${lib})
        add_library(hdf5::${lib}_${HDF5_TARGET_SUFFIX} ${HDF5_LINK_TYPE} IMPORTED)
        set_target_properties(hdf5::${lib}_${HDF5_TARGET_SUFFIX} PROPERTIES IMPORTED_LOCATION  ${HDF5_CXX_LIBRARY_${lib}})
    elseif(HDF5_Fortran_LIBRARY_${lib})
        add_library(hdf5::${lib}_${HDF5_TARGET_SUFFIX} ${HDF5_LINK_TYPE} IMPORTED)
        set_target_properties(hdf5::${lib}_${HDF5_TARGET_SUFFIX} PROPERTIES IMPORTED_LOCATION  ${HDF5_Fortran_LIBRARY_${lib}})
    elseif(HDF5_${lib}_LIBRARY) # -- For older versions of CMake
        add_library(hdf5::${lib}_${HDF5_TARGET_SUFFIX} ${HDF5_LINK_TYPE} IMPORTED)
        set_target_properties(hdf5::${lib}_${HDF5_TARGET_SUFFIX} PROPERTIES IMPORTED_LOCATION  ${HDF5_${lib}_LIBRARY})
        # Just append the usual suspects
        list(APPEND othernames z dl rt m)
        list(REMOVE_DUPLICATES othernames)
    else()
        message(STATUS "Could not match lib ${lib} and language ${lang} to a defined variable \n"
                "-- Considered in order: \n"
                "--     HDF5_${lang}_LIBRARY_${lib} \n"
                "--     HDF5_C_LIBRARY_${lib} \n"
                "--     HDF5_CXX_LIBRARY_${lib} \n"
                "--     HDF5_Fortran_LIBRARY_${lib} \n"
                "--     HDF5_${lib}_LIBRARY \n " )
        return()
    endif()


    foreach(other ${othernames})
        if("${other}" MATCHES "hdf5")
            target_link_libraries(hdf5::${lib}_${HDF5_TARGET_SUFFIX} INTERFACE hdf5::${other}_${HDF5_TARGET_SUFFIX})
        else()
            target_link_libraries(hdf5::${lib}_${HDF5_TARGET_SUFFIX} INTERFACE ${other})
        endif()
    endforeach()

    # Take care of includes
    set(${lib}_include ${HDF5_${lang}_INCLUDE_DIRS} ${HDF5_${lang}_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
    list(REMOVE_DUPLICATES ${lib}_include)
    target_include_directories(hdf5::${lib}_${HDF5_TARGET_SUFFIX} SYSTEM INTERFACE ${${lib}_include})


    set(${target_list} "${${target_list}};hdf5::${lib}_${HDF5_TARGET_SUFFIX}" PARENT_SCOPE)
endfunction()

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
        if("C" IN_LIST HDF5_COMPONENTS)
            enable_language(C)
        endif()
        find_package(HDF5 ${HDF5_ATLEAST_VERSION} COMPONENTS  ${HDF5_COMPONENTS} QUIET)
    endif()
    if(HDF5_FOUND)
        # Add C_HL and CXX_HL to the language bindings
        set(HDF5_LANG)
        if("HL" IN_LIST HDF5_COMPONENTS)
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
        #To print all variables, use the code below:
#        get_cmake_property(_variableNames VARIABLES)
#        foreach (_variableName ${_variableNames})
#            if("${_variableName}" MATCHES "HDF5|hdf5|Hdf5")
#                message(STATUS "${_variableName}=${${_variableName}}")
#            endif()
#        endforeach()

        # Get a list of library names like hdf5 hdf5_hl hdf5_hl_cpp etc
        foreach(lang ${HDF5_LANG})
#            message(lang: ${lang})
            foreach(lib ${HDF5_${lang}_LIBRARY_NAMES})
                #                message("INVESTIGATING: HDF5_${lang}_LIBRARY_${lib}: ${HDF5_${lang}_LIBRARY_${lib}} due to HDF5_${lang}_LIBRARY_NAMES : ${HDF5_${lang}_LIBRARY_NAMES}")
                if(${lib} MATCHES "hdf5")
                    list(APPEND HDF5_LIBNAMES ${lib})
                else()
                    list(APPEND HDF5_LINK_LIBNAMES ${lib})
                endif()
            endforeach()
        endforeach()

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

            set(HDF5_FOUND                  ${HDF5_FOUND}               PARENT_SCOPE)
            set(HDF5_VERSION                ${HDF5_VERSION}             PARENT_SCOPE)
            set(HDF5_IS_PARALLEL            ${HDF5_IS_PARALLEL}         PARENT_SCOPE)
            foreach(lang ${HDF5_LANG})
                if(HDF5_${lang}_LIBRARY_NAMES)
                    define_hdf5_target(${lang} "${HDF5_${lang}_LIBRARY_NAMES}" HDF5_TARGETS)
                endif()
            endforeach()
            set(HDF5_LIBRARIES          ${HDF5_LIBRARIES}           PARENT_SCOPE)
            set(HDF5_LIBNAMES           ${HDF5_LIBNAMES}            PARENT_SCOPE)
            set(HDF5_LINK_LIBNAMES      ${HDF5_LINK_LIBNAMES}       PARENT_SCOPE)
            set(HDF5_TARGETS            ${HDF5_TARGETS}             PARENT_SCOPE)
        endif()
    endif()
endfunction()




function(find_package_hdf5_exec_wrapper hdf5_paths HDF5_COMPONENTS HDF5_ATLEAST_VERSION HDF5_USE_STATIC_LIBRARIES HDF5_PREFER_PARALLEL HDF5_REQUIRED )

    foreach(hdf5_root ${hdf5_paths})
        find_package_hdf5_isolator("${hdf5_root}")
        if(HDF5_FOUND)
            set(HDF5_FOUND               ${HDF5_FOUND}                  PARENT_SCOPE)
            set(HDF5_VERSION             ${HDF5_VERSION}                PARENT_SCOPE)
            set(HDF5_IS_PARALLEL         ${HDF5_IS_PARALLEL}            PARENT_SCOPE)
            set(HDF5_TARGETS             ${HDF5_TARGETS}                PARENT_SCOPE)
            set(HDF5_LIBRARIES           ${HDF5_LIBRARIES}              PARENT_SCOPE)
            set(HDF5_LIBNAMES            ${HDF5_LIBNAMES}               PARENT_SCOPE)
            set(HDF5_LINK_LIBNAMES       ${HDF5_LINK_LIBNAMES}          PARENT_SCOPE)
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
    if(TARGET hdf5::hdf5)
        return()
    endif()

    cmake_policy(SET CMP0074 NEW)
    if(NOT HDF5_COMPONENTS)
        set(HDF5_COMPONENTS C CXX HL)
    endif()


    if(BUILD_SHARED_LIBS)
        set(HDF5_USE_STATIC_LIBRARIES OFF)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(HDF5_LINK_TYPE SHARED)
        set(HDF5_TARGET_SUFFIX shared)
        list(APPEND HDF5_COMPONENTS_CONFIG shared)
    else()
        set(HDF5_USE_STATIC_LIBRARIES ON)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(HDF5_LINK_TYPE STATIC)
        set(HDF5_TARGET_SUFFIX static)
        list(APPEND HDF5_COMPONENTS_CONFIG static)
    endif()

    if(NOT HDF5_ATLEAST_VERSION)
        set(HDF5_ATLEAST_VERSION 1.8)
#        set(HDF5_ATLEAST_VERSION 1.10)
    endif()

    if(NOT HDF5_PREFER_PARALLEL)
        set(HDF5_PREFER_PARALLEL OFF)
        list(APPEND HDF5_COMPONENTS_CONFIG parallel)
    endif()

    if (NOT HDF5_REQUIRED)
        set(HDF5_REQUIRED OFF)
    endif()

    if(NOT HDF5_FOUND)
        # Message try finding HDF5 where it would have gotten installed previously
        find_package(HDF5
                COMPONENTS ${HDF5_COMPONENTS} ${HDF5_COMPONENTS_CONFIG}
                HINTS ${hdf5_install_prefix} ${CMAKE_INSTALL_PREFIX} NO_DEFAULT_PATH)
        if(TARGET hdf5_hl_cpp-${HDF5_TARGET_SUFFIX})
            list(APPEND HDF5_TARGETS hdf5_hl_cpp-${HDF5_TARGET_SUFFIX})
        endif()
        if(TARGET hdf5_hl-${HDF5_TARGET_SUFFIX})
            list(APPEND HDF5_TARGETS hdf5_hl-${HDF5_TARGET_SUFFIX})
        endif()
        if(TARGET hdf5-${HDF5_TARGET_SUFFIX})
            list(APPEND HDF5_TARGETS hdf5-${HDF5_TARGET_SUFFIX})
        endif()

        #To print all variables, use the code below:
        #get_cmake_property(_variableNames VARIABLES)
        #foreach (_variableName ${_variableNames})
        #    if("${_variableName}" MATCHES "HDF5|hdf5|Hdf5")
        #        message(STATUS "${_variableName}=${${_variableName}}")
        #    endif()
        #endforeach()
    endif()

    if(NOT HDF5_FOUND)
        # Message try finding HDF5 using executable wrappers
        list(APPEND HDF5_PATHS
                    ${HDF5_ROOT}
                    $ENV{HDF5_ROOT}
                    ${CONAN_HDF5_ROOT}
                    $ENV{EBROOTHDF5}
                    ${H5PP_DIRECTORY_HINTS}
                    ${CMAKE_BINARY_DIR}/h5pp-deps-install
                    /usr /usr/local
                    ${CMAKE_INSTALL_PREFIX}
                    $ENV{CONDA_PREFIX}
                    $ENV{PATH})
        find_package_hdf5_exec_wrapper("${HDF5_PATHS}" "${HDF5_COMPONENTS}" "${HDF5_ATLEAST_VERSION}" "${HDF5_USE_STATIC_LIBRARIES}" "${HDF5_PREFER_PARALLEL}" "${HDF5_REQUIRED}")
    endif()


    if(HDF5_FOUND)

#        include(cmake/PrintTargetInfo.cmake)
#        message("HDF5_TARGETS: ${HDF5_TARGETS}")
#        foreach(tgt ${HDF5_TARGETS})
#            print_target_info(${tgt})
#        endforeach()

        add_library(hdf5::hdf5 IMPORTED INTERFACE)
        target_link_libraries(hdf5::hdf5 INTERFACE ${HDF5_TARGETS})

        if("sz" IN_LIST HDF5_LINK_LIBNAMES)
            CHECK_LIBRARY_EXISTS(aec aec_decode_init "/usr/lib/x86_64-linux-gnu" HAVE_AEC_LIB)
            if(HAVE_AEC_LIB)
                target_link_libraries(hdf5::hdf5 INTERFACE aec)
            endif()
        endif()
        set(HDF5_FOUND               ${HDF5_FOUND}                  PARENT_SCOPE)
        set(HDF5_VERSION             ${HDF5_VERSION}                PARENT_SCOPE)
        set(HDF5_IS_PARALLEL         ${HDF5_IS_PARALLEL}            PARENT_SCOPE)
        set(HDF5_TARGETS             "hdf5::hdf5;${HDF5_TARGETS}"   PARENT_SCOPE)
        set(HDF5_LIBRARIES           ${HDF5_LIBRARIES}              PARENT_SCOPE)
        set(HDF5_LIBNAMES            ${HDF5_LIBNAMES}               PARENT_SCOPE)
        set(HDF5_LINK_LIBNAMES       ${HDF5_LINK_LIBNAMES}          PARENT_SCOPE)
        message(STATUS "Found HDF5 ${HDF5_VERSION}")

    endif()
endfunction()

