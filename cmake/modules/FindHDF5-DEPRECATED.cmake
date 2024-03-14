
# If CMake version >= 3.19 this module will simply pass through to CMake's built-in
# module FindHDF5.cmake.
# If CMake version < 3.19 this module replaces CMake's own FindHDF5.cmake module



# The motivation for this wrapper module is that the naming conventions of HDF5 targets, variables,
# libraries (e.g. zlib,szip,aec) as well as compile options vary greatly between different
# CMake versions, HDF5 versions and build options.
# In addition, the original FindHDF5.cmake doesn't always honor the static/shared settings,
# in particular when it interrogates the h5cc compiler wrapper. This can cause shared libraries
# to be injected on static builds.

# Most of these issues are fixed from CMake version 3.19, which defines targets that are passed through
# in this wrapper.

# When HDF5 is found, a target "HDF5::HDF5" is generated so the user can simply do:
#
#       target_link_libraries(myTarget INTERFACE HDF5::HDF5)
#
# and hopefully get everything right.
#
# The call signature is the same as for the original FindHDF5.cmake bundled with CMake,
# The user can guide the behavior by setting the following variables prior to calling find_package(HDF5):
#       HDF5_ROOT                   install directory for HDF5 possible root directories to search in
#       HDF5_USE_STATIC_LIBRARIES   find static HDF5 libraries
#       HDF5_FIND_DEBUG             emit more log messages during the search process
#


function(hdf5_message level msg)
    if(HDF5_FIND_DEBUG AND level MATCHES "TRACE|DEBUG|VERBOSE|STATUS")
        message(STATUS "${msg} ${ARGN}")
    else()
        message(${level} "${msg} ${ARGN}")
    endif()
endfunction()

function(hdf5_unset_all var)
    unset(${var})
    unset(${var} CACHE)
endfunction()

function(register_standard_hdf5_target_names)
    # Map any existing HDF5 targets to one of the following
    #``hdf5::hdf5``
    #``hdf5::hdf5_hl_cpp``
    #``hdf5::hdf5_fortran``
    #``hdf5::hdf5_hl``
    #``hdf5::hdf5_hl_fortran``

    list(APPEND HDF5_C_SUFFIX "_c")
    list(APPEND HDF5_CPP_SUFFIX _cpp -cpp)
    list(APPEND HDF5_HL_SUFFIX _hl -hl)
    list(APPEND HDF5_HL_CPP_SUFFIX _cpp_hl _hl_cpp -cpp-hl -cpp_hl _cpp-hl)
    list(APPEND HDF5_FORTRAN_SUFFIX _fortran -fortran)
    list(APPEND HDF5_HL_FORTRAN_SUFFIX _fortran_hl -fortran-hl _fortran-hl -fortran_hl)
    list(APPEND HDF5_COMPONENTS CPP HL HL_CPP FORTRAN HL_FORTRAN)

    if(BUILD_SHARED_LIBS AND NOT HDF5_USE_STATIC_LIBRARIES)
        list(APPEND HDF5_LINKTYPE_SUFFIX "-shared;_shared;;")
    else()
        list(APPEND HDF5_LINKTYPE_SUFFIX "-static;_static;;")
    endif()

    foreach(CONF ${CMAKE_CONFIGURATION_TYPES};${CMAKE_BUILD_TYPE})
        string(TOLOWER "${CONF}" conf)
        list(APPEND HDF5_BUILDTYPE_SUFFIX _${CONF} -${CONF} _${conf} -${conf})
    endforeach()

    list(APPEND HDF5_NAMESPACE_PREFIX "hdf5::")

    foreach(pfx ${HDF5_NAMESPACE_PREFIX})
        foreach(bfx ${HDF5_BUILDTYPE_SUFFIX})
            foreach(lfx ${HDF5_LINKTYPE_SUFFIX})
                if(NOT TARGET hdf5::hdf5)
                    set(HDF5_C_CANDIDATES
                            ${pfx}hdf5
                            ${pfx}hdf5${lfx}${bfx}
                            ${pfx}hdf5${lfx}
                            ${pfx}hdf5${bfx}
                            ${pfx}hdf5${bfx}${lfx}
                            hdf5
                            hdf5${lfx}${bfx}
                            hdf5${lfx}
                            hdf5${bfx}
                            hdf5${bfx}${lfx}
                            )
                    foreach(tgt ${HDF5_C_CANDIDATES})
                        if(TARGET ${tgt})
                            hdf5_message(TRACE "Mapping ${tgt} --> hdf5::hdf5")
                            add_library(hdf5::hdf5 INTERFACE IMPORTED)
                            target_link_libraries(hdf5::hdf5 INTERFACE ${tgt})
                            break()
                        endif()
                    endforeach()
                endif()
                foreach(COMP ${HDF5_COMPONENTS})
                    string(TOLOWER "${COMP}" comp)
                    if(NOT TARGET hdf5::hdf5_${comp})
                        foreach(sfx ${HDF5_${COMP}_SUFFIX})
                            set(HDF5_${COMP}_CANDIDATES
                                    ${pfx}hdf5${sfx}
                                    ${pfx}hdf5${sfx}${lfx}${bfx}
                                    ${pfx}hdf5${sfx}${lfx}
                                    ${pfx}hdf5${sfx}${bfx}
                                    ${pfx}hdf5${sfx}${bfx}${lfx}
                                    hdf5${sfx}
                                    hdf5${sfx}${lfx}${bfx}
                                    hdf5${sfx}${lfx}
                                    hdf5${sfx}${bfx}
                                    hdf5${sfx}${bfx}${lfx}
                                    )
                            foreach(tgt ${HDF5_${COMP}_CANDIDATES})
                                if(TARGET ${tgt})
                                    hdf5_message(TRACE "Mapping ${tgt} --> hdf5::hdf5_${comp}")
                                    add_library(hdf5::hdf5_${comp} INTERFACE IMPORTED)
                                    target_link_libraries(hdf5::hdf5_${comp} INTERFACE ${tgt})
                                    break()
                                endif()
                            endforeach()
                            if(TARGET hdf5::hdf5_${comp})
                                break()
                            endif()
                        endforeach()
                    endif()
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endfunction()


function(get_hdf5_library_linkage libs link LINK)
    foreach(lib ${libs})
        if("${lib}" MATCHES "hdf5")
            if(TARGET ${lib})
                get_target_property(libtype ${lib} TYPE)
                if(libtype MATCHES "STATIC")
                    set(${link} static PARENT_SCOPE)
                    set(${LINK} STATIC PARENT_SCOPE)
                    return()
                elseif(libtype MATCHES "SHARED")
                    set(${link} shared PARENT_SCOPE)
                    set(${LINK} SHARED PARENT_SCOPE)
                    return()
                endif()
            else()
                get_filename_component(ext ${lib} EXT)
                if("${ext}" IN_LIST CMAKE_STATIC_LIBRARY_SUFFIX)
                    set(${link} static PARENT_SCOPE)
                    set(${LINK} STATIC PARENT_SCOPE)
                    return()
                endif()
                if("${ext}" IN_LIST CMAKE_SHARED_LIBRARY_SUFFIX)
                    set(${link} shared PARENT_SCOPE)
                    set(${LINK} SHARED PARENT_SCOPE)
                    return()
                endif()
            endif()
        endif()
    endforeach()
    set(${link} unknown PARENT_SCOPE)
    set(${LINK} UNKNOWN PARENT_SCOPE)
endfunction()


function(collect_hdf5_target_names target_list)
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
    if(NOT HDF5_USE_STATIC_LIBRARIES OR BUILD_SHARED_LIBS)
        set(HDF5_TARGET_SUFFIX -shared _shared)
    else()
        set(HDF5_TARGET_SUFFIX -static _static)
    endif()

    foreach(cmp ${HDF5_COMPONENT_NAMES})
        foreach(sfx ${HDF5_TARGET_SUFFIX})
            list(APPEND HDF5_TARGET_CANDIDATES
                    hdf5::hdf5${cmp}${sfx}
                    hdf5::hdf5${cmp}
                    hdf5::hdf5${sfx}
                    hdf5::hdf5
                    hdf5${cmp}${sfx}
                    hdf5${cmp}
                    hdf5${sfx}
                    hdf5
                    )
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES HDF5_TARGET_CANDIDATES)

    foreach(tgt ${HDF5_TARGET_CANDIDATES})
        if(TARGET ${tgt})
            hdf5_message(VERBOSE "Found target: ${tgt}")
            list(APPEND HDF5_TARGETS ${tgt})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES HDF5_TARGETS)
    set(${target_list} ${HDF5_TARGETS} PARENT_SCOPE)
endfunction()


function(define_hdf5_target lang libnames target_list)
    # Variable libnames can be a list of library names: usually, it's a list such as "hdf5;pthread;dl;m"
    # Here we extract the first element and call it lib, and collect the rest as "linklibs"
    list(GET libnames 0 lib)
    list(LENGTH libnames numlibs)
    if(numlibs GREATER 1)
        list(SUBLIST libnames 1 -1 linklibs)
    endif()


    if(HDF5_${lib}_LIBRARY AND NOT HDF5_${lang}_LIBRARY_${lib}) # This is strictly for older versions of CMake
        set(HDF5_${lang}_LIBRARY_${lib} ${HDF5_${lib}_LIBRARY})
        list(APPEND linklibs z dl rt m)  # Just append the usual suspects
        list(REMOVE_DUPLICATES linklibs)
    endif()
    if(HDF5_${lang}_LIBRARY_${lib})
        get_hdf5_library_linkage(HDF5_${lang}_LIBRARY_${lib} HDF5_${lang}_LIBRARY_${lib}_LINKAGE HDF5_${lang}_LIBRARY_${lib}_LINKAGE)
    endif()

    # Set up the dependency structure of the imported libraries
    if(NOT TARGET hdf5::${lib})
        add_library(hdf5::${lib} ${HDF5_${lang}_LIBRARY_${lib}_LINKAGE} IMPORTED)
        if(HDF5_${lang}_LIBRARY_${lib})
            set_target_properties(hdf5::${lib} PROPERTIES IMPORTED_LOCATION ${HDF5_${lang}_LIBRARY_${lib}})
        endif()
    endif()
    if(${lib} STREQUAL "hdf5")
        # Take care of includes
        set(${lib}_include ${HDF5_${lang}_INCLUDE_DIRS} ${HDF5_${lang}_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
        list(REMOVE_DUPLICATES ${lib}_include)
        target_include_directories(hdf5::${lib} SYSTEM INTERFACE ${${lib}_include})
        # Apply OS dependent fixes to the main hdf5 library (other hdf5 libraries will link to this one)
        if(MSVC AND "${HDF5_${lang}_LIBRARY_${lib}_LINKAGE}" MATCHES "SHARED")
            target_compile_definitions(hdf5::${lib} INTERFACE H5_BUILT_AS_DYNAMIC_LIB)
        endif()
    else()
        # All hdf5 libraries depend on the main c-library called hdf5::hdf5
        target_link_libraries(hdf5::${lib} INTERFACE hdf5::hdf5)
    endif()

    # Take care of HDF5 dependencies such as zlib, szip, pthreads, etc
    if(${lib} STREQUAL "hdf5")
        if("sz" IN_LIST linklibs)
            # Fix for missing aec dependency in the apt package when linking statically
            find_library(AEC_LIBRARY NAMES aec)
            CHECK_LIBRARY_EXISTS(aec aec_decode_init "/usr/lib/x86_64-linux-gnu" AEC_EXISTS)
            if(AEC_EXISTS OR AEC_LIBRARY)
                list(APPEND linklibs aec)
            endif()
            if(APPLE)
                # Fix for HDF5 from brew in macos 10.xx missing -L dir
                find_library(SZIP_LIBRARY NAMES sz szip szip-static HINTS /usr/local/opt /usr/local/lib) # No built in findSZIP.cmake
                if(SZIP_LIBRARY)
                    hdf5_message(STATUS "Found SZIP: ${SZIP_LIBRARY}")
                    get_filename_component(SZIP_PARENT_DIR ${SZIP_LIBRARY} DIRECTORY)
                    target_link_libraries(hdf5::${lib} INTERFACE -L${SZIP_PARENT_DIR})
                endif()
            endif()
        endif()

        # Handle the other link libraries in "linklibs".
        foreach(lnk ${linklibs})
            if("${lnk}" MATCHES "hdf5")
                target_link_libraries(hdf5::${lib} INTERFACE hdf5::${lnk})
            elseif(${lnk} MATCHES "pthread") # Link with -pthread instead of libpthread directly
                set(THREADS_PREFER_PTHREAD_FLAG TRUE)
                find_package(Threads REQUIRED)
                target_link_libraries(Threads::Threads INTERFACE rt dl)
                target_link_libraries(hdf5::${lib} INTERFACE Threads::Threads)
            elseif(HDF5_${lang}_LIBRARY_${lnk})
                if(NOT TARGET hdf5::${lnk})
                    if("${lnk}" STREQUAL "m" OR "${lnk}" STREQUAL "dl")
                        # dl and m have to be linked with "-ldl" or "-lm", in particular on static builds.
                        add_library(hdf5::${lnk} INTERFACE IMPORTED)
                        target_link_libraries(hdf5::${lnk} INTERFACE ${lnk})
                    else()
                        add_library(hdf5::${lnk} UNKNOWN IMPORTED)
                        set_target_properties(hdf5::${lnk} PROPERTIES IMPORTED_LOCATION ${HDF5_${lang}_LIBRARY_${lnk}})
                    endif()
                endif()
                target_link_libraries(hdf5::${lib} INTERFACE hdf5::${lnk})
            else()
                target_link_libraries(hdf5::${lib} INTERFACE ${lnk})
            endif()
        endforeach()
    endif()
    # Append to target_list
    set(${target_list} "${${target_list}};hdf5::${lib}" PARENT_SCOPE)
endfunction()


macro(define_found_components)
    set(HDF5_FOUND                ${HDF5_FOUND}               PARENT_SCOPE)
    set(HDF5_TARGETS              ${HDF5_TARGETS}             PARENT_SCOPE)
    set(HDF5_VERSION              ${HDF5_VERSION}             PARENT_SCOPE)
    set(HDF5_LIBRARIES            ${HDF5_LIBRARIES}           PARENT_SCOPE)
    set(HDF5_LIBNAMES             ${HDF5_LIBNAMES}            PARENT_SCOPE)
    set(HDF5_LINK_LIBNAMES        ${HDF5_LINK_LIBNAMES}       PARENT_SCOPE)
    set(HDF5_LANGUAGE_BINDINGS    ${HDF5_LANGUAGE_BINDINGS}   PARENT_SCOPE)
    if(HDF5_IS_PARALLEL)
        set(HDF5_IS_PARALLEL    ${HDF5_IS_PARALLEL}         PARENT_SCOPE)
    elseif(HDF5_ENABLE_PARALLEL) # Different name in config mode
        set(HDF5_IS_PARALLEL    ${HDF5_ENABLE_PARALLEL}     PARENT_SCOPE)
    endif()

    # Include dirs is special. We can get HDF5_INCLUDE_DIRS, HDF5_INCLUDE_DIR or both.
    list(APPEND HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
    list(REMOVE_DUPLICATES HDF5_INCLUDE_DIRS)
    set(HDF5_INCLUDE_DIRS       ${HDF5_INCLUDE_DIRS}        PARENT_SCOPE)

    # Handle components
    foreach(comp ${HDF5_FIND_COMPONENTS};${HDF5_LANGUAGE_BINDINGS};"")
        foreach(conf ${HDF5_COMPONENTS_CONFIG};${HDF5_TARGET_SUFFIX};"")
            foreach(comb ${comp}_${conf} ${conf}_${comp} ${comp} ${conf})
                foreach(sfx LIBRARIES LIBRARY FOUND HL_FOUND)
                    if(HDF5_${comb}_${sfx})
                        set(HDF5_${comp}_${sfx} ${HDF5_${comb}_${sfx}} PARENT_SCOPE)
                        set(HDF5_${comb}_${sfx} ${HDF5_${comb}_${sfx}} PARENT_SCOPE)
                    endif()
                endforeach()
            endforeach()
        endforeach()
        # Fortran_HL is special...
        if("${comp}" MATCHES "Fortran_HL")
            if("${HDF5_Fortran_LIBRARY_hdf5};${HDF5_TARGETS};${HDF5_LIBNAMES}" MATCHES "hl_fortran")
                set(HDF5_Fortran_HL_FOUND TRUE PARENT_SCOPE)
            endif()
        endif()
    endforeach()
endmacro()


function(find_package_hdf5_in_root hdf5_root)
    # Setup this find_package module call
    foreach(lang ${HDF5_FIND_COMPONENTS})
        hdf5_unset_all(HDF5_${lang}_COMPILER_EXECUTABLE)
    endforeach()
    hdf5_unset_all(HDF5_FOUND)
    set(HDF5_FIND_REQUIRED OFF)
    set(HDF5_NO_FIND_PACKAGE_CONFIG_FILE ON)
    hdf5_message(VERBOSE "Searching for hdf5 execs in ${hdf5_root}" )

    if(HDF5_USE_STATIC_LIBRARIES)
        set(HDF5_EXEC_TESTFLAG "-noshlib")
    else()
        set(HDF5_EXEC_TESTFLAG "-shlib")
    endif()

    if("C" IN_LIST HDF5_FIND_COMPONENTS)
        enable_language(C)
        find_program(HDF5_C_COMPILER_EXECUTABLE NAMES h5cc h5pcc HINTS ${hdf5_root} PATH_SUFFIXES hdf5 bin hdf5/bin  ${CMAKE_INSTALL_LIBEXECDIR} ${CMAKE_INSTALL_BINDIR} NO_DEFAULT_PATH)
        if(HDF5_C_COMPILER_EXECUTABLE)
            hdf5_message(VERBOSE "Found hdf5 compiler wrapper C       : ${HDF5_C_COMPILER_EXECUTABLE} ")
        endif()
    endif()
    if("CXX" IN_LIST HDF5_FIND_COMPONENTS)
        enable_language(CXX)
        find_program(HDF5_CXX_COMPILER_EXECUTABLE NAMES h5c++ HINTS ${hdf5_root} PATH_SUFFIXES hdf5 bin hdf5/bin ${CMAKE_INSTALL_LIBEXECDIR} ${CMAKE_INSTALL_BINDIR} NO_DEFAULT_PATH)
        if(HDF5_CXX_COMPILER_EXECUTABLE)
            hdf5_message(VERBOSE "Found hdf5 compiler wrapper CXX     : ${HDF5_CXX_COMPILER_EXECUTABLE}")
        endif()
    endif()
    if("Fortran" IN_LIST HDF5_FIND_COMPONENTS)
        enable_language(Fortran)
        find_program(HDF5_Fortran_COMPILER_EXECUTABLE NAMES h5fc h5pfc HINTS ${hdf5_root} PATH_SUFFIXES hdf5 bin hdf5/bin ${CMAKE_INSTALL_LIBEXECDIR} ${CMAKE_INSTALL_BINDIR} NO_DEFAULT_PATH)
        if(HDF5_Fortran_COMPILER_EXECUTABLE)
            hdf5_message(VERBOSE "Found hdf5 compiler wrapper Fortran : ${HDF5_Fortran_COMPILER_EXECUTABLE}")
        endif()
    endif()

    if (HDF5_C_COMPILER_EXECUTABLE OR HDF5_CXX_COMPILER_EXECUTABLE OR HDF5_Fortran_COMPILER_EXECUTABLE)
        hdf5_message(TRACE   "Looking for components              : ${HDF5_FIND_COMPONENTS}")
        hdf5_message(TRACE   "find_package(HDF5) in MODULE mode...")
        set(CMAKE_MODULE_PATH "") # Modify this variable locally to make sure we use CMake's inbuilt FindHDF5.cmake module
        find_package(HDF5 ${HDF5_FIND_VERSION} COMPONENTS ${HDF5_FIND_COMPONENTS} QUIET)
        hdf5_message(DEBUG "find_package(HDF5) in MODULE mode... Found: ${HDF5_FOUND}")
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

        # When HDF5 is built with CMake the compiler wrappers sometimes not linked properly.
        # This causes the FindHDF5.cmake module bundled with CMake to give a false positive,
        # claiming that the libraries are found when they actually aren't.
        # We can notice when this happens by checking the return values defined in find_package.
        # Read more here
        # https://gitlab.kitware.com/cmake/cmake/-/issues/20387


        # In the rare cases the executable wrapper is broken we only get the
        # hdf5 libraries but not the extra libraries like m, dl, z and sz.
        # Here we take care of this case by checking if they exist and adding
        # them to the list of libraries manually.

        if(HDF5_C_RETURN_VALUE OR HDF5_CXX_RETURN_VALUE OR HDF5_Fortran_RETURN_VALUE)
            hdf5_message(WARNING "One or more HDF5 compiler wrappers may have failed")
            foreach(lang ${HDF5_LANG})
                foreach(lib m dl z sz aec)
                    find_library(${lib}_var NAMES ${lib} QUIET)
                    if(lib)
                        list(APPEND HDF5_${lang}_LIBRARY_NAMES $<LINK_ONLY:${lib}>)
                    endif()
                endforeach()
                hdf5_message(VERBOSE "Added link libraries: ${HDF5_${lang}_LIBRARY_NAMES}")
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
                get_hdf5_library_linkage("${HDF5_${lang}_LIBRARY_${lib}}" link_type LINK_TYPE)
                if(HDF5_USE_STATIC_LIBRARIES AND ${LINK_TYPE} MATCHES "SHARED")
                    set(ACCEPT_PACKAGE FALSE)
                    set(REJECT_REASON "Found shared versions of HDF5 libraries despite having set HDF5_USE_STATIC_LIBRARIES: ${HDF5_USE_STATIC_LIBRARIES}")
                    list(APPEND HDF5_WRONG_LIBS ${HDF5_${lang}_LIBRARY_${lib}})
                endif()
            endforeach()
        endforeach()

        # Check that each extra link library (not the hdf5 libraries!) have correct static/shared extension
        foreach(lang ${HDF5_LANG})
            foreach(lib ${HDF5_LINK_LIBNAMES})
                get_filename_component(lib_extension "${HDF5_${lang}_LIBRARY_${lib}}" EXT)
                if(HDF5_USE_STATIC_LIBRARIES AND NOT lib_extension MATCHES "${HDF5_LIBRARY_SUFFIX}" )
                    # If we reached this point, the package wants to inject shared libraries
                    # We need to check that each library is available as a static library when linking as "-l<name>" or with "-L <libpath>"
                    # or else we have to reject this package.
                    hdf5_unset_all(HDF5_LINK_LIBNAME_${lib})
                    find_library(HDF5_LINK_LIBNAME_${lib} NAMES ${lib} HINTS /usr/local/opt QUIET)
                    if(HDF5_LINK_LIBNAME_${lib})
                        get_filename_component(HDF5_LINK_LIBNAME_${lib}_DIR ${HDF5_LINK_LIBNAME_${lib}} DIRECTORY)
                        list(APPEND HDF5_LINK_LIBDIRS -L${HDF5_LINK_LIBNAME_${lib}_DIR})
                    else()
                        set(ACCEPT_PACKAGE FALSE)
                        set(REJECT_REASON "HDF5 static libraries depends on a shared library and no static replacement was found")
                        list(APPEND HDF5_WRONG_LIBS ${lib})
                    endif()
                endif()
            endforeach()
        endforeach()


        if (ACCEPT_PACKAGE)
            if(NOT HDF5_ROOT AND HDF5_CXX_COMPILER_EXECUTABLE)
                get_filename_component(HDF5_ROOT "${HDF5_CXX_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            endif()
            if(NOT HDF5_ROOT AND HDF5_C_COMPILER_EXECUTABLE)
                get_filename_component(HDF5_ROOT "${HDF5_C_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            endif()
            if(NOT HDF5_ROOT AND HDF5_Fortran_COMPILER_EXECUTABLE)
                get_filename_component(HDF5_ROOT "${HDF5_Fortran_COMPILER_EXECUTABLE}/../.." ABSOLUTE)
            endif()
            foreach(lang ${HDF5_LANG})
                if(HDF5_${lang}_LIBRARY_NAMES)
                    define_hdf5_target(${lang} "${HDF5_${lang}_LIBRARY_NAMES}" HDF5_TARGETS)
                endif()
            endforeach()
            define_found_components()
        else()
            list(REMOVE_DUPLICATES HDF5_WRONG_LIBS)
            hdf5_message(STATUS "HDF5 Package disqualified: ${HDF5_VERSION} ${HDF5_INCLUDE_DIR}\n \
            Reason: ${REJECT_REASON}: ${HDF5_WRONG_LIBS}" )
            #Unset variables so that we start from a clean search next time
            get_cmake_property(_variableNames VARIABLES)
            foreach (_variableName ${_variableNames})
                if("${_variableName}" MATCHES "HDF5_")
                    hdf5_unset_all(${_variableName})
                endif()
            endforeach()
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

    include(GNUInstallDirs)
    foreach(hdf5_root ${HDF5_PATHS})
        find_package_hdf5_in_root("${hdf5_root}")
        if(HDF5_FOUND)
            define_found_components()
            return()
        else()
            set(HDF5_FOUND FALSE PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

function(find_package_hdf5)
    # Unset cached variables to make sure we end up calling find_package at least once
    foreach(tgt ${HDF5_TARGETS})
        if(NOT TARGET ${tgt})
            hdf5_unset_all(HDF5_TARGETS)
            hdf5_unset_all(HDF5_FOUND)
            break()
        endif()
    endforeach()

    # Configure shared/static linkage

    if(HDF5_USE_STATIC_LIBRARIES)
        set(HDF5_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()

    # Setup components
    if(NOT HDF5_FIND_COMPONENTS)
        list(APPEND HDF5_FIND_COMPONENTS C HL)
    endif()

    # Find HDF5
    if(NOT HDF5_FOUND)
        find_package_hdf5_exec_wrapper()
    endif()
    define_found_components()
endfunction()





if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    set(HANDLE_VERSION_RANGE HANDLE_VERSION_RANGE)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(HDF5
            REQUIRED_VARS HDF5_INCLUDE_DIRS HDF5_TARGETS
            VERSION_VAR HDF5_VERSION
            HANDLE_COMPONENTS
            HANDLE_VERSION_RANGE
            )
else()
    find_package_hdf5()
endif()

# To print all variables, use the code below:
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    if("${_variableName}" MATCHES "HDF5|hdf5|Hdf5")
#        message(STATUS "${_variableName}=${${_variableName}}")
#    endif()
#endforeach()
#


if(HDF5_FOUND)

    # Sanity check
    foreach(tgt ${HDF5_TARGETS})
        if(NOT TARGET ${tgt})
            hdf5_message(FATAL_ERROR "Found target [${tgt}] is not a valid target. This is a bug in FindHDF5.cmake and should not happen.")
        endif()
    endforeach()

    # Generate interface libraries with everything needed
    get_hdf5_library_linkage("${HDF5_TARGETS}" hdf5_link_type HDF5_LINK_TYPE)
    if(NOT TARGET HDF5::HDF5)
        add_library(HDF5::HDF5 INTERFACE IMPORTED)
    endif()
    # Overwrite any existing link libraries, otherwise we risk linking to libdl.a rather than -ldl, or both, instance.
    set_target_properties(HDF5::HDF5 PROPERTIES INTERFACE_LINK_LIBRARIES "${HDF5_TARGETS}")

    if(NOT TARGET HDF5::ALIAS)
        add_library(HDF5::ALIAS INTERFACE IMPORTED)
        target_link_libraries(HDF5::ALIAS INTERFACE HDF5::HDF5)
    endif()
    if(NOT TARGET hdf5::all) # Deprecated, kept for compatibility
        add_library(hdf5::all  INTERFACE IMPORTED)
        target_link_libraries(hdf5::all INTERFACE HDF5::HDF5)
    endif()


    # Set variables to match the signature of the original cmake-bundled FindHDF5.cmake
    foreach(tgt ${HDF5_TARGETS})
        if(${tgt} MATCHES "cpp_hl|hl_cpp")
            set(HDF5_CXX_HL_TARGETS ${tgt})
        elseif(${tgt} MATCHES "cpp")
            set(HDF5_CXX_TARGETS ${tgt})
        elseif(${tgt} MATCHES "fortran")
            set(HDF5_Fortran_TARGETS ${tgt})
        elseif(${tgt} MATCHES "hl")
            set(HDF5_C_HL_TARGETS ${tgt})
        elseif(${tgt} MATCHES "static|shared")
            set(HDF5_C_TARGETS ${tgt})
        endif()
    endforeach()

    foreach(cmp C CXX C_HL CXX_HL Fortran)
        foreach(tgt ${HDF5_${type}_TARGETS})
            if(TARGET ${tgt})
                get_target_property(IMPLIB ${tgt} LOCATION)
                if(IMPLIB)
                    list(APPEND HDF5_${cmp}_LIBRARY ${IMPLIB} )
                    hdf5_message(VERBOSE "Found HDF5 target ${tgt} for HDF5_${cmp}_LIBRARY: ${HDF5_${cmp}_LIBRARY}")
                endif()
            endif()
        endforeach()
    endforeach()
endif()
