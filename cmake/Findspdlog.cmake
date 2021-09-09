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

if(NOT spdlog_FIND_VERSION)
    if(NOT spdlog_FIND_VERSION_MAJOR)
        set(spdlog_FIND_VERSION_MAJOR 1)
    endif()
    if(NOT spdlog_FIND_VERSION_MINOR)
        set(spdlog_FIND_VERSION_MINOR 3)
    endif()
    if(NOT spdlog_FIND_VERSION_PATCH)
        set(spdlog_FIND_VERSION_PATCH 1)
    endif()
    set(spdlog_FIND_VERSION "${spdlog_FIND_VERSION_MAJOR}.${spdlog_FIND_VERSION_MINOR}.${spdlog_FIND_VERSION_PATCH}")
endif()

function(spdlog_check_version_include incdir)
    if (IS_DIRECTORY "${incdir}")
        set(include ${incdir})
    elseif(IS_DIRECTORY "${${incdir}}")
        set(include ${${incdir}})
    else()
        return()
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
    endif()

    if(DEFINED spdlog_FIND_VERSION)
        if(SPDLOG_VERSION VERSION_GREATER_EQUAL spdlog_FIND_VERSION)
            set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
            set(SPDLOG_VERSION_OK TRUE PARENT_SCOPE)
        else()
            set(SPDLOG_VERSION_OK FALSE PARENT_SCOPE)
        endif()
    else()
        set(SPDLOG_VERSION_OK TRUE PARENT_SCOPE)
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


function(find_spdlog_config)
    if(NOT BUILD_SHARED_LIBS)
        # Spdlog from ubuntu apt injects shared library into static builds.
        # Can't take any chances here.
        set(NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH)
        set(NO_SYSTEM_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
    # First try finding a config somewhere in the system
    if(NOT SPDLOG_NO_CONFIG OR SPDLOG_CONFIG_ONLY)
        find_package(spdlog ${spdlog_FIND_VERSION}
                HINTS ${spdlog_FIND_HINTS} ${H5PP_DEPS_INSTALL_DIR}
                PATH_SUFFIXES ${spdlog_FIND_PATH_SUFFIXES} include spdlog include/spdlog spdlog/include/spdlog
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
                        "However, the target spdlog::spdlog has already been defined, so it will be used:"
                        "SPDLOG_VERSION:     ${SPDLOG_VERSION}"
                        "SPDLOG_INCLUDE_DIR: ${SPDLOG_INCLUDE_DIR}"
                        "Something is wrong with your installation of spdlog")
                set(SPDLOG_VERSION_OK TRUE)
            endif()
            if(SPDLOG_INCLUDE_DIR MATCHES "conda")
                # Use the header-only mode to avoid weird linking errors
                target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_HEADER_ONLY)
            endif()
        endif()
    endif()
    set(SPDLOG_INCLUDE_DIR ${SPDLOG_INCLUDE_DIR} PARENT_SCOPE)
    set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
    set(SPDLOG_VERSION_OK ${SPDLOG_VERSION_OK} PARENT_SCOPE)
endfunction()

function(find_spdlog_manual)
    if(NOT TARGET spdlog::spdlog AND NOT SPDLOG_CONFIG_ONLY)
        if(NOT BUILD_SHARED_LIBS)
            # Spdlog from ubuntu apt injects shared library into static builds.
            # Can't take any chances here.
            set(NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH})
            set(NO_SYSTEM_ENVIRONMENT_PATH  NO_SYSTEM_ENVIRONMENT_PATH)
            set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
        endif()
        find_path(SPDLOG_INCLUDE_DIR
                NAMES spdlog/spdlog.h
                HINTS ${H5PP_DEPS_INSTALL_DIR} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES spdlog/include include spdlog include/spdlog spdlog/include/spdlog
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_SYSTEM_PATH}
                ${NO_SYSTEM_ENVIRONMENT_PATH}
                QUIET
                )
        # There may o compiled library to go with the headers
        find_library(SPDLOG_LIBRARY
                NAMES spdlog
                HINTS ${SPDLOG_INCLUDE_DIR}
                PATH_SUFFIXES spdlog/lib
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_PACKAGE_REGISTRY}
                ${NO_CMAKE_SYSTEM_PATH}
                ${NO_SYSTEM_ENVIRONMENT_PATH}
                )

        spdlog_check_version_include(SPDLOG_INCLUDE_DIR)
        if(SPDLOG_VERSION_OK AND SPDLOG_INCLUDE_DIR)
            add_library(spdlog::spdlog INTERFACE IMPORTED)
            add_library(spdlog::spdlog_header_only INTERFACE IMPORTED)
            target_include_directories(spdlog::spdlog SYSTEM INTERFACE ${SPDLOG_INCLUDE_DIR})
            target_include_directories(spdlog::spdlog_header_only SYSTEM INTERFACE ${SPDLOG_INCLUDE_DIR})

            if(SPDLOG_LIBRARY AND NOT SPDLOG_LIBRARY MATCHES "conda")
                target_link_libraries(spdlog::spdlog INTERFACE ${SPDLOG_LIBRARY} )
                target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_COMPILED_LIB )
            else()
                # Choose header-only because conda libraries sometimes give linking errors, such as:
                # /usr/bin/ld:
                #       /home/user/miniconda/lib/libspdlog.a(spdlog.cpp.o): TLS transition from R_X86_64_TLSLD
                #       to R_X86_64_TPOFF32 against `_ZGVZN6spdlog7details2os9thread_idEvE3tid'
                #       at 0x4 in section `.text._ZN6spdlog7details2os9thread_idEv' failed
                # /home/user/miniconda/lib/libspdlog.a: error adding symbols: Bad value
                target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_HEADER_ONLY)
            endif()
        endif()
    endif()
    set(SPDLOG_INCLUDE_DIR ${SPDLOG_INCLUDE_DIR} PARENT_SCOPE)
    set(SPDLOG_VERSION ${SPDLOG_VERSION} PARENT_SCOPE)
    set(SPDLOG_VERSION_OK ${SPDLOG_VERSION_OK} PARENT_SCOPE)
endfunction()

function(set_spdlog_version vers)
    if(CMAKE_VERSION VERSION_LESS 3.19)
        return()
    endif()
    if(TARGET spdlog::spdlog)
        get_target_property(tgt_alias spdlog::spdlog ALIASED_TARGET )
        if(tgt_alias)
            set_target_properties(${tgt_alias} PROPERTIES VERSION ${${vers}})
        else()
            set_target_properties(spdlog::spdlog PROPERTIES VERSION ${${vers}})
        endif()
    endif()
endfunction()

function(spdlog_link_fmt)
    if(TARGET spdlog::spdlog)
        find_package(fmt QUIET)
        if(fmt_FOUND AND TARGET fmt::fmt)
            get_target_property(tgt_alias spdlog::spdlog ALIASED_TARGET )
            get_target_property(FMT_INCLUDE_DIR fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
            if(tgt_alias)
                set(tgt ${tgt_alias})
            else()
                set(tgt spdlog::spdlog)
            endif()
            target_link_libraries(${tgt} INTERFACE fmt::fmt)
            if(NOT FMT_INCLUDE_DIR MATCHES "bundled")
                    target_compile_definitions(${tgt} INTERFACE SPDLOG_FMT_EXTERNAL)
            endif()
        endif()
    endif()
endfunction()

find_spdlog_config()
find_spdlog_manual()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
        REQUIRED_VARS SPDLOG_INCLUDE_DIR SPDLOG_VERSION_OK
        VERSION_VAR SPDLOG_VERSION
        FAIL_MESSAGE "Failed to find spdlog"
        )

if(spdlog_FOUND)
    set_spdlog_version(SPDLOG_VERSION)
    spdlog_link_fmt()
endif()