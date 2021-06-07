# This sets the following variables

# SPDLOG_INCLUDE_DIR
# SPDLOG_VERSION
# spdlog_FOUND

# as well as a target spdlog::spdlog
#
# To guide the find behavior the user can set the following variables to TRUE/FALSE:

# SPDLOG_NO_CMAKE_PACKAGE_REGISTRY
# SPDLOG_NO_DEFAULT_PATH
# SPDLOG_NO_CONFIG
# SPDLOG_CONFIG_ONLY

# The user can set search directory hints from CMake or environment, such as
# spdlog_DIR, spdlog_ROOT, etc.


if(SPDLOG_NO_DEFAULT_PATH)
    set(NO_DEFAULT_PATH NO_DEFAULT_PATH)
endif()

if(SPDLOG_NO_CMAKE_PACKAGE_REGISTRY)
    set(NO_CMAKE_PACKAGE_REGISTRY NO_CMAKE_PACKAGE_REGISTRY)
endif()

if(SPDLOG_NO_CMAKE_SYSTEM_PATH)
    set(NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH)
endif()
if(SPDLOG_NO_SYSTEM_ENVIRONMENT_PATH)
    set(NO_SYSTEM_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH)
endif()
if(NOT BUILD_SHARED_LIBS)
    # Spdlog from ubuntu apt injects shared library into static builds.
    # Can't take any chances here.
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

if(NOT spdlog_FIND_VERSION)
    if(NOT spdlog_FIND_VERSION_MAJOR)
        set(spdlog_FIND_VERSION_MAJOR 1)
    endif()
    if(NOT spdlog_FIND_VERSION_MINOR)
        set(spdlog_FIND_VERSION_MINOR 1)
    endif()
    if(NOT spdlog_FIND_VERSION_PATCH)
        set(spdlog_FIND_VERSION_PATCH 0)
    endif()
    set(spdlog_FIND_VERSION "${spdlog_FIND_VERSION_MAJOR}.${spdlog_FIND_VERSION_MINOR}.${spdlog_FIND_VERSION_PATCH}")
endif()


function(spdlog_check_version_include incdir)
    if (IS_DIRECTORY "${incdir}")
        set(include ${incdir})
    elseif(IS_DIRECTORY "${${incdir}}")
        set(include ${${incdir}})
    endif()
    if(EXISTS ${include}/spdlog/version.h)
        file(READ "${include}/spdlog/version.h" _spdlog_version_header)
        string(REGEX MATCH "define[ \t]+SPDLOG_VER_MAJOR[ \t]+([0-9]+)" _spdlog_world_version_match "${_spdlog_version_header}")
        set(SPDLOG_WORLD_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+SPDLOG_VER_MINOR[ \t]+([0-9]+)" _spdlog_major_version_match "${_spdlog_version_header}")
        set(SPDLOG_MAJOR_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+SPDLOG_VER_PATCH[ \t]+([0-9]+)" _spdlog_minor_version_match "${_spdlog_version_header}")
        set(SPDLOG_MINOR_VERSION "${CMAKE_MATCH_1}")

        set(SPDLOG_VERSION ${SPDLOG_WORLD_VERSION}.${SPDLOG_MAJOR_VERSION}.${SPDLOG_MINOR_VERSION})
        message(STATUS "Found Spdlog version ${SPDLOG_VERSION}")
    endif()


    if(SPDLOG_VERSION VERSION_GREATER_EQUAL spdlog_FIND_VERSION)
        set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
        set(SPDLOG_VERSION_OK TRUE PARENT_SCOPE)
    else()
        set(SPDLOG_VERSION_OK FALSE PARENT_SCOPE)
    endif()
endfunction()


function(spdlog_check_version_include_genexp genexp_incdir)
    string(REGEX REPLACE "BUILD_INTERFACE|INSTALL_INTERFACE|<|>|:" ";" incdirs "${${genexp_incdir}}")
    foreach(inc ${incdirs})
        if(inc STREQUAL "$") # The regex does not match dollar signs in generator expressions
            continue()
        endif()
        spdlog_check_version_include(${inc})
        if(SPDLOG_VERSION_OK)
            set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
            set(SPDLOG_VERSION_OK TRUE PARENT_SCOPE)
            break()
        endif()
    endforeach()
endfunction()

function(spdlog_check_version_target tgt)
    if(TARGET ${tgt})
        get_target_property(SPDLOG_VERSION ${tgt} VERSION)
        get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
        set(SPDLOG_INCLUDE_DIR ${SPDLOG_INCLUDE_DIR} PARENT_SCOPE)
        if(SPDLOG_VERSION VERSION_GREATER_EQUAL spdlog_FIND_VERSION)
            set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
            set(SPDLOG_VERSION_OK TRUE PARENT_SCOPE)
        else()
            spdlog_check_version_include_genexp(SPDLOG_INCLUDE_DIR)
            set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
            set(SPDLOG_VERSION_OK ${SPDLOG_VERSION_OK} PARENT_SCOPE)
        endif()
    endif()
endfunction()

# First try finding a config somewhere in the system
if(NOT SPDLOG_NO_CONFIG OR SPDLOG_CONFIG_ONLY)
    find_package(spdlog ${spdlog_FIND_VERSION}
            HINTS ${H5PP_DEPS_INSTALL_DIR} ${CONAN_SPDLOG_ROOT} ${CMAKE_INSTALL_PREFIX} ${H5PP_DEPS_INSTALL_DIR}
            PATH_SUFFIXES include spdlog include/spdlog spdlog/include/spdlog
            ${NO_DEFAULT_PATH}
            ${NO_CMAKE_PACKAGE_REGISTRY}
            ${NO_CMAKE_SYSTEM_PATH}
            ${NO_SYSTEM_ENVIRONMENT_PATH}
            CONFIG QUIET
            )
    if(TARGET spdlog::spdlog)
        spdlog_check_version_target(spdlog::spdlog)
        if(NOT SPDLOG_VERSION_OK OR NOT SPDLOG_VERSION)
            message(WARNING "Could not determine the spdlog version.\n"
                    "However, the target spdlog::spdlog has already been defined, so it will be used:\n"
                    "SPDLOG_INCLUDE_DIR: ${SPDLOG_INCLUDE_DIR}\n"
                    "SPDLOG_VERSION:     ${SPDLOG_VERSION}\n"
                    "Something is wrong with your installation of spdlog")
        endif()
        if(SPDLOG_INCLUDE_DIR MATCHES "conda")
            # Use the header-only mode to avoid weird linking errors
            target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_HEADER_ONLY)
        endif()
    endif()
endif()

if(NOT TARGET spdlog::spdlog AND NOT SPDLOG_CONFIG_ONLY)
    find_path(SPDLOG_INCLUDE_DIR
            NAMES spdlog/spdlog.h
            HINTS ${H5PP_DEPS_INSTALL_DIR} ${CONAN_SPDLOG_ROOT} ${CMAKE_INSTALL_PREFIX}
            PATH_SUFFIXES spdlog/include include spdlog include/spdlog spdlog/include/spdlog
            ${NO_DEFAULT_PATH}
            ${NO_CMAKE_PACKAGE_REGISTRY}
            ${NO_CMAKE_SYSTEM_PATH}
            ${NO_SYSTEM_ENVIRONMENT_PATH}
            QUIET
            )
    if(SPDLOG_INCLUDE_DIR)
        spdlog_check_version_include(SPDLOG_INCLUDE_DIR)
        if(SPDLOG_VERSION_OK)
            set(spdlog_FOUND TRUE)
            add_library(spdlog::spdlog INTERFACE IMPORTED)
            target_include_directories(spdlog::spdlog SYSTEM INTERFACE ${SPDLOG_INCLUDE_DIR})
            if(SPDLOG_INCLUDE_DIR MATCHES "conda")
                # Choose header-only because conda libraries sometimes give linking errors, such as:
                # /usr/bin/ld:
                #       /home/user/miniconda/lib/libspdlog.a(spdlog.cpp.o): TLS transition from R_X86_64_TLSLD
                #       to R_X86_64_TPOFF32 against `_ZGVZN6spdlog7details2os9thread_idEvE3tid'
                #       at 0x4 in section `.text._ZN6spdlog7details2os9thread_idEv' failed
                # /home/user/miniconda/lib/libspdlog.a: error adding symbols: Bad value
                target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_HEADER_ONLY)
            else()
                # There may or may not be a compiled library to go with the headers
                include(GNUInstallDirs)
                find_library(SPDLOG_LIBRARY
                        NAMES spdlog
                        HINTS ${SPDLOG_INCLUDE_DIR}
                        PATH_SUFFIXES spdlog/${CMAKE_INSTALL_LIBDIR}  ${CMAKE_INSTALL_LIBDIR}
                        ${NO_DEFAULT_PATH}
                        ${NO_CMAKE_PACKAGE_REGISTRY}
                        ${NO_CMAKE_SYSTEM_PATH}
                        ${NO_SYSTEM_ENVIRONMENT_PATH}
                        )
                if(SPDLOG_LIBRARY AND NOT SPDLOG_LIBRARY MATCHES "conda")
                    target_link_libraries(spdlog::spdlog INTERFACE ${SPDLOG_LIBRARY} )
                    target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_COMPILED_LIB )
                else()
                    target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_HEADER_ONLY)
                endif()
            endif()
        endif()
    endif()
endif()

function(target_set_version tgt vers)
    if(TARGET ${tgt})
        get_target_property(tgt_alias ${tgt} ALIASED_TARGET )
        if(tgt_alias)
            set_target_properties(${tgt_alias} PROPERTIES VERSION ${vers})
        else()
            set_target_properties(${tgt} PROPERTIES VERSION ${vers})
        endif()
    endif()
endfunction()

function(target_link_fmt tgt)
    if(TARGET ${tgt})
        find_package(fmt 6.1.2 QUIET)
        if(fmt_FOUND AND TARGET fmt::fmt)
            get_target_property(tgt_alias ${tgt} ALIASED_TARGET )
            if(tgt_alias)
                target_link_libraries(${tgt_alias} INTERFACE fmt::fmt)
            else()
                target_link_libraries(${tgt} INTERFACE fmt::fmt)
            endif()

            if(NOT FMT_INCLUDE_DIR)
                get_target_property(FMT_INCLUDE_DIR fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
            endif()
            if(NOT FMT_INCLUDE_DIR MATCHES "bundled")
                if(tgt_alias)
                    target_compile_definitions(${tgt_alias} INTERFACE SPDLOG_FMT_EXTERNAL)
                else()
                    target_compile_definitions(${tgt} INTERFACE SPDLOG_FMT_EXTERNAL)
                endif()
            endif()
        endif()
    endif()
endfunction()


if(TARGET spdlog::spdlog AND SPDLOG_VERSION AND SPDLOG_VERSION_OK)
    set(spdlog_FOUND TRUE)
    target_set_version(spdlog::spdlog SPDLOG_VERSION)
    target_link_fmt(spdlog::spdlog)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
        FOUND_VAR spdlog_FOUND
        REQUIRED_VARS SPDLOG_INCLUDE_DIR SPDLOG_VERSION_OK
        VERSION_VAR SPDLOG_VERSION
        FAIL_MESSAGE "Failed to find spdlog"
        )

mark_as_advanced(SPDLOG_INCLUDE_DIR)
mark_as_advanced(spdlog_FOUND)