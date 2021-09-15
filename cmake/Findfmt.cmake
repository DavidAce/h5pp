# This sets the following variables

# FMT_INCLUDE_DIR
# FMT_VERSION
# fmt_FOUND

# as well as a target fmt::fmt
#
# To guide the find behavior the user can set the following variables to TRUE/FALSE:

# FMT_NO_CMAKE_PACKAGE_REGISTRY
# FMT_NO_DEFAULT_PATH
# FMT_NO_CONFIG
# FMT_CONFIG_ONLY

# The user can set search directory hints from CMake or environment, such as
# fmt_DIR, fmt_ROOT, etc.

if(FMT_NO_DEFAULT_PATH)
    set(NO_DEFAULT_PATH NO_DEFAULT_PATH)
endif()

if(FMT_NO_CMAKE_PACKAGE_REGISTRY)
    set(NO_CMAKE_PACKAGE_REGISTRY NO_CMAKE_PACKAGE_REGISTRY)
endif()

if(FMT_NO_CMAKE_SYSTEM_PATH)
    set(NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH)
endif()
if(FMT_NO_SYSTEM_ENVIRONMENT_PATH)
    set(NO_SYSTEM_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH)
endif()

if(NOT fmt_FIND_VERSION)
    if(NOT fmt_FIND_VERSION_MAJOR)
        set(fmt_FIND_VERSION_MAJOR 5)
    endif()
    if(NOT fmt_FIND_VERSION_MINOR)
        set(fmt_FIND_VERSION_MINOR 0)
    endif()
    if(NOT fmt_FIND_VERSION_PATCH)
        set(fmt_FIND_VERSION_PATCH 0)
    endif()
    set(fmt_FIND_VERSION "${fmt_FIND_VERSION_MAJOR}.${fmt_FIND_VERSION_MINOR}.${fmt_FIND_VERSION_PATCH}")
endif()

function(fmt_check_version_include incdir)
    if (IS_DIRECTORY "${incdir}")
        set(include ${incdir})
    elseif(IS_DIRECTORY "${${incdir}}")
        set(include ${${incdir}})
    else()
        return()
    endif()
    if(EXISTS ${include}/fmt/core.h)
        set(_fmt_version_file "${include}/fmt/core.h")
    elseif(EXISTS ${include}/fmt/bundled/core.h)
        set(_fmt_version_file "${include}/fmt/bundled/core.h")
    endif()
    if(EXISTS ${_fmt_version_file})
        # parse "#define FMT_VERSION 40100" to 4.1.0
        file(STRINGS "${_fmt_version_file}" FMT_VERSION_LINE REGEX "^#define[ \t]+FMT_VERSION[ \t]+[0-9]+$")
        string(REGEX REPLACE "^#define[ \t]+FMT_VERSION[ \t]+([0-9]+)$" "\\1" FMT_VERSION "${FMT_VERSION_LINE}")
        foreach(ver "FMT_VERSION_PATCH" "FMT_VERSION_MINOR" "FMT_VERSION_MAJOR")
            math(EXPR ${ver} "${FMT_VERSION} % 100")
            math(EXPR FMT_VERSION "(${FMT_VERSION} - ${${ver}}) / 100")
        endforeach()
        set(FMT_VERSION "${FMT_VERSION_MAJOR}.${FMT_VERSION_MINOR}.${FMT_VERSION_PATCH}")
    endif()

    if(FMT_VERSION VERSION_GREATER_EQUAL fmt_FIND_VERSION)
        set(FMT_VERSION ${FMT_VERSION} PARENT_SCOPE)
        set(FMT_VERSION_OK TRUE PARENT_SCOPE)
    else()
        set(FMT_VERSION_OK FALSE PARENT_SCOPE)
    endif()
endfunction()


function(fmt_check_version_include_genexp genexp_incdir)
    string(REGEX REPLACE "BUILD_INTERFACE|INSTALL_INTERFACE|<|>|:" ";" incdirs "${${genexp_incdir}}")
    foreach(inc ${incdirs})
        if(inc STREQUAL "$") # The regex does not match dollar signs in generator expressions
            continue()
        endif()
        fmt_check_version_include(${inc})
        if(FMT_VERSION_OK)
            set(FMT_VERSION ${FMT_VERSION} PARENT_SCOPE)
            set(FMT_VERSION_OK TRUE PARENT_SCOPE)
            break()
        endif()
    endforeach()
endfunction()

function(fmt_check_version_target tgt)
    if(TARGET ${tgt})
        get_target_property(FMT_VERSION ${tgt} VERSION)
        get_target_property(FMT_INCLUDE_DIR fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
        set(FMT_INCLUDE_DIR ${FMT_INCLUDE_DIR} PARENT_SCOPE)
        if(FMT_VERSION VERSION_GREATER_EQUAL fmt_FIND_VERSION)
            set(FMT_VERSION ${FMT_VERSION} PARENT_SCOPE)
            set(FMT_VERSION_OK TRUE PARENT_SCOPE)
        else()
            fmt_check_version_include_genexp(FMT_INCLUDE_DIR)
            set(FMT_VERSION ${FMT_VERSION} PARENT_SCOPE)
            set(FMT_VERSION_OK ${FMT_VERSION_OK} PARENT_SCOPE)
        endif()
    endif()
endfunction()


function(find_fmt_config)
    # Try to find a config somewhere in the system
    find_package(fmt ${fmt_FIND_VERSION}
            HINTS ${fmt_FIND_HINTS} ${H5PP_DEPS_INSTALL_DIR}
            PATH_SUFFIXES include fmt include/fmt fmt/include/fmt
            ${NO_DEFAULT_PATH}
            ${NO_CMAKE_PACKAGE_REGISTRY}
            ${NO_CMAKE_SYSTEM_PATH}
            ${NO_SYSTEM_ENVIRONMENT_PATH}
            CONFIG QUIET
            )
    if(TARGET fmt::fmt)
        fmt_check_version_target(fmt::fmt)
        if(NOT FMT_VERSION_OK OR NOT FMT_VERSION)
            message(WARNING "Could not determine the version of fmt.\n"
                    "However, the target fmt::fmt has already been defined, so it will be used:\n"
                    "FMT_INCLUDE_DIR: ${FMT_INCLUDE_DIR}\n"
                    "FMT_VERSION:     ${FMT_VERSION}\n"
                    "Something is wrong with your installation of fmt")
            set(FMT_VERSION_OK TRUE)
        endif()

        if(FMT_INCLUDE_DIR MATCHES "conda")
            # Use the header-only mode to avoid weird linking errors
            target_compile_definitions(fmt::fmt INTERFACE FMT_HEADER_ONLY)
        endif()
    endif()
    set(FMT_INCLUDE_DIR ${FMT_INCLUDE_DIR} PARENT_SCOPE)
    set(FMT_VERSION ${FMT_VERSION} PARENT_SCOPE)
    set(FMT_VERSION_OK ${FMT_VERSION_OK} PARENT_SCOPE)
endfunction()

function(find_fmt_manual)
    if(NOT TARGET fmt::fmt AND NOT FMT_CONFIG_ONLY)
        find_path(FMT_INCLUDE_DIR
                fmt/core.h
                HINTS ${H5PP_DEPS_INSTALL_DIR} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES fmt fmt/include include include/fmt
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_SYSTEM_PATH}
                ${NO_SYSTEM_ENVIRONMENT_PATH}
                )
        find_library(FMT_LIBRARY
                NAMES fmt
                HINTS ${H5PP_DEPS_INSTALL_DIR} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES fmt fmt/lib
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_SYSTEM_PATH}
                ${NO_SYSTEM_ENVIRONMENT_PATH}
                )

        fmt_check_version_include(FMT_INCLUDE_DIR)

        if(FMT_VERSION_OK AND FMT_INCLUDE_DIR)
            add_library(fmt::fmt INTERFACE IMPORTED)
            add_library(fmt::fmt-header-only INTERFACE IMPORTED)
            target_include_directories(fmt::fmt SYSTEM INTERFACE ${FMT_INCLUDE_DIR})
            target_include_directories(fmt::fmt-header-only SYSTEM INTERFACE ${FMT_INCLUDE_DIR})

            target_compile_definitions(fmt::fmt-header-only INTERFACE FMT_HEADER_ONLY=1)
            target_compile_features(fmt::fmt INTERFACE cxx_variadic_templates)
            target_compile_features(fmt::fmt-header-only INTERFACE cxx_variadic_templates)

            if(FMT_LIBRARY AND NOT FMT_LIBRARY MATCHES "conda")
                set_target_properties(fmt::fmt PROPERTIES IMPORTED_LOCATION ${FMT_LIBRARY})
            else()
                # Choose header-only because conda libraries sometimes give linking errors
                target_compile_definitions(fmt::fmt INTERFACE FMT_HEADER_ONLY=1)
            endif()
        endif()
    endif()
endfunction()

function(set_fmt_version tgt_name vers)
    if(CMAKE_VERSION VERSION_LESS 3.19)
        return()
    endif()

    if(TARGET ${tgt_name})
        get_target_property(tgt_alias ${tgt_name} ALIASED_TARGET)
        if(tgt_alias)
            set(tgt ${tgt_alias})
        else()
            set(tgt ${tgt_name})
        endif()
        set_target_properties(${tgt} PROPERTIES VERSION ${${vers}})
        if(${${vers}} VERSION_LESS 7.0.0)
            # Fix bug with ambiguous resolution of fmt::format_to which is used in spdlog 1.5.0
            target_compile_definitions(${tgt} INTERFACE FMT_USE_CONSTEXPR=0)
        endif()
    endif()
endfunction()

find_fmt_config()
find_fmt_manual()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(fmt
        REQUIRED_VARS FMT_INCLUDE_DIR FMT_VERSION_OK
        VERSION_VAR FMT_VERSION
        FAIL_MESSAGE "Failed to find fmt"
        )

if(fmt_FOUND)
    set_fmt_version(fmt::fmt FMT_VERSION)
    set_fmt_version(fmt::fmt-header-only FMT_VERSION)
endif()