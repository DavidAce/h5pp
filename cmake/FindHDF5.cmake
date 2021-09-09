

# This module wraps CMake's inbuilt FindHDF5.cmake module
# The motivation is that the naming conventions of targets, variable names,
# the required link libraries (like aec,dl,zlib,szip, etc) and compile options
# for the many library components vary greatly between different CMake versions,
# HDF5 versions and how HDF5 was compiled. In addition, the inbuilt FindHDF5.cmake
# does not always honor the static/shared setting, which may crash during linking
# due to mixing shared and static libraries.

# When HDF5 is found by this wrapper, a target "hdf5::all" is generated so the user can simply do:
#
#       target_link_libraries(myExe INTERFACE hdf5::all)
#
# and hopefully get everything right.
#
# The call signature is the same as for the original FindHDF5.cmake bundled with CMake,
# The user can guide the behavior by setting the following variables prior to calling find_package(HDF5):
#       HDF5_PATHS                  list of possible root directories to search in
#       BUILD_SHARED_LIBS           ON/OFF for shared/static libs (stricly enforced)
#
#

function(register_hdf5_targets target_list)
    if("CXX" IN_LIST HDF5_FIND_COMPONENTS)
        list(APPEND HDF5_COMPONENT_NAMES _cpp )
    endif()
    if("CXX_HL" IN_LIST HDF5_FIND_COMPONENTS)
        list(APPEND HDF5_COMPONENT_NAMES _cpp_hl _hl_cpp )
    endif()
    if("Fortran" IN_LIST HDF5_FIND_COMPONENTS)
        list(APPEND HDF5_COMPONENT_NAMES _fortran )
    endif()
    if("HL" IN_LIST HDF5_FIND_COMPONENTS)
        list(APPEND HDF5_COMPONENT_NAMES _hl)
    endif()
    if("Fortran_HL" IN_LIST "${HDF5_FIND_COMPONENTS}")
        list(APPEND HDF5_COMPONENT_NAMES hl_fortran _hl_fortran )
    endif()

    if("C" IN_LIST HDF5_FIND_COMPONENTS)
        list(APPEND HDF5_TARGET_CANDIDATES
                hdf5-${HDF5_TARGET_SUFFIX}
                hdf5_${HDF5_TARGET_SUFFIX}
                hdf5
                )
    endif()

    foreach(cmp ${HDF5_COMPONENT_NAMES})
        list(APPEND HDF5_TARGET_CANDIDATES
                hdf5::hdf5${cmp}-${HDF5_TARGET_SUFFIX}
                hdf5::hdf5${cmp}_${HDF5_TARGET_SUFFIX}
                hdf5::hdf5${cmp}
                hdf5::hdf5-${HDF5_TARGET_SUFFIX}
                hdf5::hdf5_${HDF5_TARGET_SUFFIX}
                hdf5::hdf5
                hdf5${cmp}-${HDF5_TARGET_SUFFIX}
                hdf5${cmp}_${HDF5_TARGET_SUFFIX}
                hdf5${cmp}
                hdf5
                )
    endforeach()
    list(REMOVE_DUPLICATES HDF5_TARGET_CANDIDATES)
    foreach(tgt ${HDF5_TARGET_CANDIDATES})
        if(TARGET ${tgt})
            if(HDF5_FIND_VERBOSE)
                message(STATUS "Found target: ${tgt}")
            endif()
            list(APPEND HDF5_TARGETS ${tgt})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES HDF5_TARGETS)
    set(${target_list} ${HDF5_TARGETS} PARENT_SCOPE)
endfunction()


function(define_hdf5_target lang libnames target_list)
    # Variable libnames can be a list of library names.
    # Usually, it's a list such as "hdf5;pthread;dl;m"
    # Here we extract the first element and call it lib, and collect the rest as "othernames"
    list(GET libnames 0 lib)
    list(LENGTH libnames numlibs)
    if(numlibs GREATER 1)
        list(SUBLIST libnames 1 -1 othernames)
    endif()


    if(TARGET hdf5::${lib})
        # From CMake version > 3.19 we get proper targets defined in hdf5::${lib}. There is no need to manually
        # model the HDF5 the interdependencies of the imported libraries in these targets, since that is done for us.
        # Therefore we just append the HDF5 target to the target list.
        # Note that hdf5::hdf5 is now the main C library.
        if(hdf5::${lib} IN_LIST ${target_list})
            # We already processed this target
            return()
        endif()
        if(NOT ${lib} STREQUAL "hdf5" AND TARGET hdf5::hdf5)
            # All libraries depend on the main c-library called hdf5::hdf5
            target_link_libraries(hdf5::${lib} INTERFACE hdf5::hdf5)
        endif()
    else()
        # Start modeling the dependency structure of the imported libraries
        if(HDF5_C_LIBRARY_${lib})
            add_library(hdf5::${lib} ${HDF5_LINK_TYPE} IMPORTED)
            set_target_properties(hdf5::${lib} PROPERTIES IMPORTED_LOCATION  ${HDF5_C_LIBRARY_${lib}} INTERFACE_LINK_LIBRARIES  "hdf5::hdf5")
        elseif(HDF5_CXX_LIBRARY_${lib})
            add_library(hdf5::${lib} ${HDF5_LINK_TYPE} IMPORTED)
            set_target_properties(hdf5::${lib} PROPERTIES IMPORTED_LOCATION  ${HDF5_CXX_LIBRARY_${lib}} INTERFACE_LINK_LIBRARIES "hdf5::hdf5_cpp;hdf5::hdf5}")
        elseif(HDF5_Fortran_LIBRARY_${lib})
            add_library(hdf5::${lib} ${HDF5_LINK_TYPE} IMPORTED)
            set_target_properties(hdf5::${lib} PROPERTIES IMPORTED_LOCATION  ${HDF5_Fortran_LIBRARY_${lib}} INTERFACE_LINK_LIBRARIES "hdf5::hdf5_fortran;hdf5::hdf5")
        elseif(HDF5_${lib}_LIBRARY) # -- For older versions of CMake
            add_library(hdf5::${lib} ${HDF5_LINK_TYPE} IMPORTED)
            set_target_properties(hdf5::${lib} PROPERTIES IMPORTED_LOCATION  ${HDF5_${lib}_LIBRARY})
            # Just append the usual suspects
            list(APPEND othernames z dl rt m)
            list(REMOVE_DUPLICATES othernames)
        else()
#            # To print all variables, use the code below:
#            get_cmake_property(_variableNames VARIABLES)
#            foreach (_variableName ${_variableNames})
#                if("${_variableName}" MATCHES "HDF5|hdf5|Hdf5")
#                    message(STATUS "${_variableName}=${${_variableName}}")
#                endif()
#            endforeach()
            message(STATUS "Could not match lib ${lib} and language ${lang} to a defined variable \n"
                    "-- Considered in order: \n"
                    "--     HDF5_${lang}_LIBRARY_${lib} \n"
                    "--     HDF5_C_LIBRARY_${lib} \n"
                    "--     HDF5_CXX_LIBRARY_${lib} \n"
                    "--     HDF5_Fortran_LIBRARY_${lib} \n"
                    "--     HDF5_${lib}_LIBRARY \n " )
            return()
        endif()
        # Take care of includes
        set(${lib}_include ${HDF5_${lang}_INCLUDE_DIRS} ${HDF5_${lang}_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
        list(REMOVE_DUPLICATES ${lib}_include)
        target_include_directories(hdf5::${lib} SYSTEM INTERFACE ${${lib}_include})
    endif()

    message(STATUS "Found native lib: ${libnames}")

    # The other external link libraries in "othernames" can be linked as interface libraries
    foreach(other ${othernames})
        if("${other}" MATCHES "hdf5")
            target_link_libraries(hdf5::${lib} INTERFACE hdf5::${other})
        else()
            target_link_libraries(hdf5::${lib} INTERFACE ${other})
        endif()
    endforeach()
    # Append to target_list
    set(${target_list} "${${target_list}};hdf5::${lib}" PARENT_SCOPE)


endfunction()


macro(register_found_components)
    foreach(cmp ${HDF5_FIND_COMPONENTS})
        if(HDF5_${cmp}_FOUND OR HDF5_${HDF5_TARGET_SUFFIX}_${cmp}_FOUND)
            set(HDF5_${cmp}_FOUND TRUE PARENT_SCOPE)
        endif()
        if(HDF5_HL_${cmp}_FOUND OR HDF5_${cmp}_HL_FOUND OR HDF5_${HDF5_TARGET_SUFFIX}_${cmp}_HL_FOUND)
            set(HDF5_${cmp}_HL_FOUND TRUE PARENT_SCOPE)
        endif()
        # Fortran_HL is special...
        if("${cmp}" MATCHES "Fortran_HL")
            if(HDF5_Fortran_LIBRARY_hdf5hl_fortran OR "${HDF5_TARGETS}" MATCHES "hl_fortran" OR "${HDF5_LIBNAMES}" MATCHES "hl_fortran")
                set(HDF5_Fortran_HL_FOUND TRUE PARENT_SCOPE)
            endif()
        endif()
    endforeach()
endmacro()



function(find_package_hdf5_isolator hdf5_root)
    unset(HDF5_C_COMPILER_EXECUTABLE   CACHE)
    unset(HDF5_CXX_COMPILER_EXECUTABLE CACHE)
    unset(HDF5_Fortran_COMPILER_EXECUTABLE CACHE)
    unset(HDF5_C_COMPILER_EXECUTABLE)
    unset(HDF5_CXX_COMPILER_EXECUTABLE)
    unset(HDF5_Fortran_COMPILER_EXECUTABLE)
    unset(HDF5_FOUND )
    unset(HDF5_FOUND CACHE)
    unset(HDF5_FOUND PARENT_SCOPE)
    set(HDF5_FOUND OFF)
    set(HDF5_FIND_REQUIRED OFF)
    set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE ON)
    if(HDF5_FIND_VERBOSE)
        message(STATUS "Searching for hdf5 execs in ${hdf5_root}" )
    endif()
    include(GNUInstallDirs)

    set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE ON)
    if(BUILD_SHARED_LIBS)
        set(HDF5_EXEC_TESTFLAG "-shlib")
    else()
        set(HDF5_EXEC_TESTFLAG "-noshlib")
    endif()
    if("C" IN_LIST HDF5_FIND_COMPONENTS)
        enable_language(C)
        find_file(HDF5_C_COMPILER_EXECUTABLE        NAMES h5cc h5pcc  PATHS ${hdf5_root} ${hdf5_root}/hdf5 PATH_SUFFIXES bin hdf5/bin ${CMAKE_INSTALL_LIBEXECDIR} ${CMAKE_INSTALL_BINDIR} NO_DEFAULT_PATH)
    endif()
    if("CXX" IN_LIST HDF5_FIND_COMPONENTS)
        enable_language(CXX)
        find_file(HDF5_CXX_COMPILER_EXECUTABLE      NAMES h5c++ PATHS ${hdf5_root} ${hdf5_root}/hdf5 PATH_SUFFIXES bin hdf5/bin ${CMAKE_INSTALL_LIBEXECDIR} ${CMAKE_INSTALL_BINDIR} NO_DEFAULT_PATH)
    endif()
    if("Fortran" IN_LIST HDF5_FIND_COMPONENTS)
        enable_language(Fortran)
        find_file(HDF5_Fortran_COMPILER_EXECUTABLE  NAMES h5fc h5pfc PATHS ${hdf5_root} ${hdf5_root}/hdf5 PATH_SUFFIXES bin hdf5/bin ${CMAKE_INSTALL_LIBEXECDIR} ${CMAKE_INSTALL_BINDIR} NO_DEFAULT_PATH)
    endif()

    if (HDF5_C_COMPILER_EXECUTABLE OR HDF5_CXX_COMPILER_EXECUTABLE)
        if(HDF5_FIND_VERBOSE)
            message(STATUS "Found hdf5 compiler wrapper C       : ${HDF5_C_COMPILER_EXECUTABLE} ")
            message(STATUS "Found hdf5 compiler wrapper CXX     : ${HDF5_CXX_COMPILER_EXECUTABLE}")
            message(STATUS "Found hdf5 compiler wrapper Fortran : ${HDF5_Fortran_COMPILER_EXECUTABLE}")
            message(STATUS "Looking for components              : ${HDF5_FIND_COMPONENTS}")
        endif()

        if(HDF5_FIND_DEBUG OR HDF5_FIND_VERBOSE)
            message(STATUS "Starting find_package in MODULE mode")
        endif()
        set(CMAKE_MODULE_PATH "") # Modify this variable locally to make sure we use CMake's inbuilt FindHDF5.cmake module
        find_package(HDF5 ${HDF5_FIND_VERSION} COMPONENTS ${HDF5_FIND_COMPONENTS} QUIET)
        if(HDF5_FIND_DEBUG OR HDF5_FIND_VERBOSE)
            message(STATUS "Starting find_package(HDF5) in MODULE mode... Found: ${HDF5_FOUND}")
        endif()
    endif()
    if(HDF5_FOUND)
        # Add C_HL and CXX_HL to the language bindings
        foreach(lang ${HDF5_LANGUAGE_BINDINGS})
            string(TOUPPER ${lang} LANG)
            list(APPEND HDF5_LANG ${LANG} ${lang})
            if("HL" IN_LIST HDF5_FIND_COMPONENTS)
                list(APPEND HDF5_LANG ${LANG}_HL ${lang}_HL)
            endif()
        endforeach()
        list(REMOVE_DUPLICATES HDF5_LANG)
        set(ACCEPT_PACKAGE TRUE)
        set(HDF5_LIBNAMES)
        set(HDF5_LINK_LIBNAMES)

        # When HDF5 is built with CMake the compiler wrappers are not linked properly.
        # This causes the FindHDF5.cmake module bundled with CMake to give a false positive,
        # claiming that the libraries are found when they actually aren't.
        # Read more here
        # https://gitlab.kitware.com/cmake/cmake/-/issues/20387

        # In the rare cases the executable wrapper is broken we only the
        # hdf5 libraries but not the extra libraries like m, dl, z and sz.
        # Here we take care of this case by checking if they exist and adding
        # them to the list of libraries manually


        if(HDF5_C_RETURN_VALUE OR HDF5_CXX_RETURN_VALUE OR HDF5_Fortran_RETURN_VALUE)
            message(WARNING "One or more HDF5 compiler wrappers may have failed")
            foreach(lang ${HDF5_LANG})
                foreach(lib m dl z sz aec)
                    find_library(${lib}_var NAMES ${lib} QUIET)
                    if(lib)
                        list(APPEND HDF5_${lang}_LIBRARY_NAMES $<LINK_ONLY:${lib}>)
                    endif()
                endforeach()
                message(STATUS "Added link libraries: ${HDF5_${lang}_LIBRARY_NAMES}")
            endforeach()
        endif()


        # Get a list of library names like hdf5 hdf5_hl hdf5_hl_cpp etc
        foreach(lang ${HDF5_LANG})
            foreach(lib ${HDF5_${lang}_LIBRARY_NAMES})
                if(${lib} MATCHES "hdf5")
                    list(APPEND HDF5_LIBNAMES ${lib})
                else()
                    list(APPEND HDF5_LINK_LIBNAMES ${lib})
                endif()
            endforeach()
        endforeach()

        # Check that each hdf5 library has correct static/shared extension
        foreach(lang ${HDF5_LANG} )
            foreach(lib ${HDF5_LIBNAMES})
                get_filename_component(lib_extension "${HDF5_${lang}_LIBRARY_${lib}}" EXT)
                if(NOT BUILD_SHARED_LIBS AND NOT ${lib_extension} MATCHES "${HDF5_LIBRARY_SUFFIX}" )
                    set(ACCEPT_PACKAGE FALSE)
                    set(REJECT_REASON "Found shared versions of HDF5 libraries during static build")
                    list(APPEND HDF5_SHARED_LIBS ${HDF5_${lang}_LIBRARY_${lib}})
                endif()
            endforeach()
        endforeach()

        # Check that each link library (not hdf5 library!) has correct static/shared extension
        foreach(lang ${HDF5_LANG})
            foreach(lib ${HDF5_LINK_LIBNAMES})
                get_filename_component(lib_extension "${HDF5_${lang}_LIBRARY_${lib}}" EXT)
                if(NOT BUILD_SHARED_LIBS AND NOT lib_extension MATCHES "${HDF5_LIBRARY_SUFFIX}" )
                    # If we reached this point, the package wants to inject shared libraries to a static build
                    # We need to check that each library is available as a static library when linking as "-l<name>" or with "-L <libpath>"
                    # or else we have to reject this package.
                    unset(HDF5_LINK_LIBNAME_${lib})
                    unset(HDF5_LINK_LIBNAME_${lib} CACHE)
                    find_library(HDF5_LINK_LIBNAME_${lib} NAMES ${lib} HINTS /usr/local/opt)
                    if(HDF5_LINK_LIBNAME_${lib})
                        get_filename_component(HDF5_LINK_LIBNAME_${lib}_DIR ${HDF5_LINK_LIBNAME_${lib}} DIRECTORY)
#                        target_link_libraries(hdf5::hdf5 INTERFACE -L${HDF5_LINK_LIBNAME_${lib}_DIR})
#                        list(TRANSFORM HDF5_LINK_LIBNAMES REPLACE "${lib}" "-L${HDF5_LINK_LIBNAME_${lib}_DIR}")
                        list(APPEND HDF5_LINK_LIBDIRS -L${HDF5_LINK_LIBNAME_${lib}_DIR})
                    else()
                        set(ACCEPT_PACKAGE FALSE)
                        set(REJECT_REASON "HDF5 static libraries depends on a shared library and no static replacement could be found")
                        list(APPEND HDF5_SHARED_LIBS ${lib})
                    endif()
                endif()
            endforeach()
        endforeach()


        if (NOT ACCEPT_PACKAGE)
            list(REMOVE_DUPLICATES HDF5_SHARED_LIBS)
            message(STATUS "HDF5 Package disqualified: ${HDF5_VERSION} ${HDF5_INCLUDE_DIR}\n"
                    "       Reason: ${REJECT_REASON}: ${HDF5_SHARED_LIBS}")
            #Unset variables so that we start from a clean search next time
            get_cmake_property(_variableNames VARIABLES)
            foreach (_variableName ${_variableNames})
                if("${_variableName}" MATCHES "HDF5_")
                    unset(${_variableName})
                    unset(${_variableName} CACHE)
                endif()
            endforeach()
        else()
            get_filename_component(HDF5_ROOT "${HDF5_CXX_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            if(NOT HDF5_ROOT)
                get_filename_component(HDF5_ROOT "${HDF5_C_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            endif()

            foreach(lang ${HDF5_LANG})
                if(HDF5_${lang}_LIBRARY_NAMES)
                    define_hdf5_target(${lang} "${HDF5_${lang}_LIBRARY_NAMES}" HDF5_TARGETS)
                endif()
            endforeach()
            set(HDF5_FOUND              ${HDF5_FOUND}               PARENT_SCOPE)
            set(HDF5_VERSION            ${HDF5_VERSION}             PARENT_SCOPE)
            set(HDF5_IS_PARALLEL        ${HDF5_IS_PARALLEL}         PARENT_SCOPE)
            set(HDF5_LIBRARIES          ${HDF5_LIBRARIES}           PARENT_SCOPE)
            set(HDF5_LIBNAMES           ${HDF5_LIBNAMES}            PARENT_SCOPE)
            set(HDF5_LINK_LIBNAMES      ${HDF5_LINK_LIBNAMES}       PARENT_SCOPE)
            set(HDF5_TARGETS            ${HDF5_TARGETS}             PARENT_SCOPE)
            set(HDF5_LANGUAGE_BINDINGS  ${HDF5_LANGUAGE_BINDINGS}   PARENT_SCOPE)
            register_found_components()
        endif()
    endif()
endfunction()



function(find_package_hdf5_exec_wrapper)
    # Find HDF5 using executable wrappers
    list(APPEND HDF5_PATHS
            ${HDF5_ROOT}
            $ENV{HDF5_ROOT}
            ${HDF5_PREFIX_PATH}
            ${EBROOTHDF5})
    if(NOT HDF5_NO_CMAKE_PATH)
        list(APPEND HDF5_PATHS ${CMAKE_PREFIX_PATH})
    endif()
    if(NOT HDF5_NO_SYSTEM_ENVIRONMENT_PATH)
        if(DEFINED ENV{PATH})
            string(REPLACE ":" ";" ENVPATH $ENV{PATH})
        endif()
        list(APPEND HDF5_PATHS ${ENVPATH})
    endif()
    if(NOT HDF5_NO_CMAKE_SYSTEM_PATH)
        list(APPEND HDF5_PATHS ${CMAKE_SYSTEM_PREFIX_PATH})
    endif()

    list(REMOVE_DUPLICATES HDF5_PATHS)

    foreach(hdf5_root ${HDF5_PATHS})
        find_package_hdf5_isolator("${hdf5_root}")
        if(HDF5_FOUND)
            set(HDF5_FOUND               ${HDF5_FOUND}                  PARENT_SCOPE)
            set(HDF5_VERSION             ${HDF5_VERSION}                PARENT_SCOPE)
            set(HDF5_IS_PARALLEL         ${HDF5_IS_PARALLEL}            PARENT_SCOPE)
            set(HDF5_TARGETS             ${HDF5_TARGETS}                PARENT_SCOPE)
            set(HDF5_LIBRARIES           ${HDF5_LIBRARIES}              PARENT_SCOPE)
            set(HDF5_LIBNAMES            ${HDF5_LIBNAMES}               PARENT_SCOPE)
            set(HDF5_LINK_LIBNAMES       ${HDF5_LINK_LIBNAMES}          PARENT_SCOPE)
            set(HDF5_LANGUAGE_BINDINGS   ${HDF5_LANGUAGE_BINDINGS}      PARENT_SCOPE)
            register_found_components()
            return()
        else()
            set(HDF5_FOUND FALSE PARENT_SCOPE)
        endif()
    endforeach()
endfunction()



function(find_package_hdf5_config_wrapper)

    # We unset the specified version because on some installations, hdf5-config.cmake will mark
    # a new library (like 1.12) as incompatible with an older (like 1.8) even though h5pp is compatible with both.
    unset(HDF5_FIND_VERSION) # The user has probably installed the latest version
    unset(HDF5_FIND_VERSION_COMPLETE) # The user has probably installed the latest version
    unset(HDF5_FIND_REQUIRED) # There is a chance we may find the library using the executable wrappers

    if(HDF5_FIND_DEBUG OR HDF5_FIND_VERBOSE)
        message(STATUS "Finding package HDF5 in CONFIG mode...")
    endif()
    # Honor the HDF5_NO_<option> flags
    list(APPEND NO_OPTIONS
            NO_PACKAGE_ROOT_PATH
            NO_CMAKE_PATH
            NO_CMAKE_PACKAGE_REGISTRY
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
            NO_DEFAULT_PATH
            )

    foreach(opt ${NO_OPTIONS})
        if(DEFINED HDF5_${opt} AND NOT DEFINED ${opt})
            set(${opt} ${opt})
            if(HDF5_FIND_VERBOSE)
                message(STATUS "HDF5 find policy: ${${opt}}")
            endif()
        endif()
    endforeach()
    find_package(HDF5
            ${HDF5_FIND_VERSION}
            COMPONENTS ${HDF5_FIND_COMPONENTS} ${HDF5_COMPONENTS_CONFIG}
            HINTS  ${HDF5_ROOT}  ${H5PP_DEPS_INSTALL_DIR} ${CMAKE_INSTALL_PREFIX}
            PATH_SUFFIXES  hdf5
            # The following flags are enabled with HDF5_NO_... before calling find_package(HDF5)
            ${NO_PACKAGE_ROOT_PATH}
            ${NO_SYSTEM_ENVIRONMENT_PATH}
            ${NO_CMAKE_PATH}
            ${NO_CMAKE_PACKAGE_REGISTRY}
            ${NO_CMAKE_SYSTEM_PATH}
            ${NO_CMAKE_SYSTEM_PACKAGE_REGISTRY}
            ${NO_DEFAULT_PATH} # If enabled, this will ignore HDF5_ROOT
            CONFIG)

    #To print all variables, use the code below:
    #    get_cmake_property(_variableNames VARIABLES)
    #    foreach (_variableName ${_variableNames})
    #        if("${_variableName}" MATCHES "HDF5|hdf5|Hdf5|package|PACKAGE|Package")
    #            message(STATUS "${_variableName}=${${_variableName}}")
    #        endif()
    #    endforeach()


    if(HDF5_FOUND)
        register_hdf5_targets(HDF5_TARGETS)
        register_found_components()
        set(HDF5_FOUND               ${HDF5_FOUND}                  PARENT_SCOPE)
        set(HDF5_VERSION             ${HDF5_VERSION}                PARENT_SCOPE)
        set(HDF5_IS_PARALLEL         ${HDF5_ENABLE_PARALLEL}        PARENT_SCOPE) # In config-mode this gets a different name
        set(HDF5_TARGETS             ${HDF5_TARGETS}                PARENT_SCOPE)
        set(HDF5_LIBRARIES           ${HDF5_LIBRARIES}              PARENT_SCOPE)
        set(HDF5_LIBNAMES            ${HDF5_LIBNAMES}               PARENT_SCOPE)
        set(HDF5_LINK_LIBNAMES       ${HDF5_LINK_LIBNAMES}          PARENT_SCOPE)
        set(HDF5_LANGUAGE_BINDINGS   ${HDF5_LANGUAGE_BINDINGS}      PARENT_SCOPE)
        return()
    else()
        set(HDF5_FOUND FALSE PARENT_SCOPE)
    endif()
endfunction()


# Start finding HDF5

function(find_hdf5)

    if(BUILD_SHARED_LIBS)
        set(HDF5_USE_STATIC_LIBRARIES OFF)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(HDF5_LINK_TYPE SHARED)
        set(HDF5_TARGET_SUFFIX shared)
        list(APPEND HDF5_COMPONENTS_CONFIG shared)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(HDF5_USE_STATIC_LIBRARIES ON)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(HDF5_LINK_TYPE STATIC)
        set(HDF5_TARGET_SUFFIX static)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
        list(APPEND HDF5_COMPONENTS_CONFIG static)
    endif()

    if(NOT HDF5_FIND_VERSION)
        set(HDF5_FIND_VERSION 1.8)
    endif()

    if(NOT HDF5_FIND_DEBUG)
        set(HDF5_FIND_DEBUG OFF)
    endif()
    if(NOT HDF5_FIND_VERBOSE)
        set(HDF5_FIND_VERBOSE OFF)
    endif()

    if(NOT HDF5_FOUND)
        # Try finding HDF5 where it would have been installed previously by h5pp
        find_package_hdf5_config_wrapper()
    endif()
    if(NOT HDF5_FOUND)
        # Try finding HDF5 using executable wrappers
        find_package_hdf5_exec_wrapper()
    endif()


    if(HDF5_FOUND)
        if(NOT TARGET hdf5::all)
            add_library(hdf5::all INTERFACE IMPORTED)
        endif()

        target_link_libraries(hdf5::all INTERFACE ${HDF5_TARGETS})
        if(MSVC AND BUILD_SHARED_LIBS)
            target_compile_definitions(hdf5::all INTERFACE H5_BUILT_AS_DYNAMIC_LIB)
        endif()
        if(APPLE AND "sz" IN_LIST HDF5_LINK_LIBNAMES)
            find_library(SZIP_LIBRARY NAMES sz szip szip-static HINTS /usr/local/opt /usr/local/lib) # No built in findSZIP.cmake
            if(SZIP_LIBRARY)
                message(STATUS "Found SZIP: ${SZIP_LIBRARY}")
                get_filename_component(SZIP_PARENT_DIR ${SZIP_LIBRARY} DIRECTORY)
                target_link_libraries(hdf5::all INTERFACE -L${SZIP_PARENT_DIR})
            endif()
        endif()
        if("sz" IN_LIST HDF5_LINK_LIBNAMES)
            CHECK_LIBRARY_EXISTS(aec aec_decode_init "/usr/lib/x86_64-linux-gnu" HAVE_AEC_LIB)
            find_library(AEC_LIBRARY NAMES aec)
            if(HAVE_AEC_LIB OR AEC_LIBRARY)
                target_link_libraries(hdf5::all INTERFACE aec)
            endif()
        endif()

        if(HDF5_FIND_VERBOSE)
            if(EXISTS cmake/PrintTargetInfo.cmake)
                include(cmake/PrintTargetInfo.cmake)
                message(STATUS "HDF5_TARGETS: ${HDF5_TARGETS}")
                foreach(tgt ${HDF5_TARGETS})
                    print_target_info(${tgt})
                endforeach()
                print_target_info(hdf5::all)
            endif()
        endif()

        foreach(tgt ${HDF5_TARGETS})
            get_target_property(INCDIR ${tgt}  INTERFACE_INCLUDE_DIRECTORIES)
            if(INCDIR)
                list(APPEND HDF5_INCLUDE_DIR ${INCDIR})
            endif()
        endforeach()
        list(REMOVE_DUPLICATES HDF5_INCLUDE_DIR)
        target_include_directories(hdf5::all SYSTEM INTERFACE ${HDF5_INCLUDE_DIR})
        set(HDF5_TARGETS hdf5::all ${HDF5_TARGETS})
        if(HDF5_PREFER_PARALLEL)
            message(STATUS "Found HDF5: ${HDF5_INCLUDE_DIR} (found version ${HDF5_VERSION}) | Parallel: ${HDF5_IS_PARALLEL}")
        endif()

        # Set variables to match the signature of the original cmake-bundled FindHDF5.cmake
        set(HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
        foreach(tgt ${HDF5_TARGETS})
            if(${tgt} MATCHES "cpp_hl|hl_cpp")
                list(APPEND HDF5_CXX_HL_TARGETS ${tgt})
            elseif(${tgt} MATCHES "cpp")
                list(APPEND HDF5_CXX_TARGETS ${tgt})
            elseif(${tgt} MATCHES "fortran")
                list(APPEND HDF5_Fortran_TARGETS ${tgt})
            elseif(${tgt} MATCHES "hl")
                list(APPEND HDF5_C_HL_TARGETS ${tgt})
            elseif(${tgt} MATCHES "static|shared")
                list(APPEND HDF5_C_TARGETS ${tgt})
            endif()
        endforeach()
        foreach(type C CXX C_HL CXX_HL Fortran)
            foreach(tgt ${HDF5_${type}_TARGETS})
                if(TARGET ${tgt})
                    get_target_property(IMPLIB ${tgt} LOCATION)
                    if(IMPLIB)
                        list(APPEND HDF5_${type}_LIBRARIES ${IMPLIB} )
                    endif()
                    if(INTLIB)
                        list(APPEND HDF5_${type}_LIBRARIES ${INTLIB} )
                    endif()
                    if(HDF5_FIND_VERBOSE OR HDF5_FIND_DEBUG)
                        message(STATUS "Detected HDF5 ${type} component target ${tgt} with HDF5_${type}_LIBRARIES: ${HDF5_${type}_LIBRARIES}")
                    endif()
                endif()
            endforeach()
        endforeach()
    endif()
    set(HDF5_FOUND              ${HDF5_FOUND}               PARENT_SCOPE)
    set(HDF5_C_FOUND            ${HDF5_C_FOUND}             PARENT_SCOPE)
    set(HDF5_CXX_FOUND          ${HDF5_CXX_FOUND}           PARENT_SCOPE)
    set(HDF5_Fortran_FOUND      ${HDF5_Fortran_FOUND}       PARENT_SCOPE)
    set(HDF5_C_HL_FOUND         ${HDF5_C_HL_FOUND}          PARENT_SCOPE)
    set(HDF5_CXX_HL_FOUND       ${HDF5_CXX_HL_FOUND}        PARENT_SCOPE)
    set(HDF5_Fortran_HL_FOUND   ${HDF5_Fortran_HL_FOUND}    PARENT_SCOPE)
    set(HDF5_HL_FOUND           ${HDF5_HL_FOUND}            PARENT_SCOPE)
    set(HDF5_HL_CXX_FOUND       ${HDF5_HL_CXX_FOUND}        PARENT_SCOPE)
    set(HDF5_HL_Fortran_FOUND   ${HDF5_HL_Fortran_FOUND}    PARENT_SCOPE)

    set(HDF5_INCLUDE_DIR        ${HDF5_INCLUDE_DIR}         PARENT_SCOPE)
    set(HDF5_TARGETS            ${HDF5_TARGETS}             PARENT_SCOPE)
    set(HDF5_VERSION            ${HDF5_VERSION}             PARENT_SCOPE)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(HDF5
            FOUND_VAR HDF5_FOUND
            REQUIRED_VARS HDF5_INCLUDE_DIR HDF5_TARGETS
            VERSION_VAR HDF5_VERSION
            HANDLE_COMPONENTS
            FAIL_MESSAGE "Failed to find HDF5"
            )
endfunction()


#if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
#    set(CMAKE_MODULE_PATH "") # Modify this variable locally to make sure we use CMake's inbuilt FindHDF5.cmake module
#    enable_language(C)
#    find_package(HDF5) # Other arguments have already been parsed and are forwarded to this find_package call
#else()
#endif()

find_hdf5()
