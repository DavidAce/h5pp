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

function(find_fmt)
    find_path(FMT_INCLUDE_DIR
                fmt/core.h
                HINTS ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES fmt fmt/include include include/fmt
                )
    if(FMT_INCLUDE_DIR)
        find_library(FMT_LIBRARY
                NAMES fmt
                HINTS ${FMT_INCLUDE_DIR} ${FMT_INCLUDE_DIR}../ ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES lib fmt fmt/lib
                )
    fmt_check_version_include(FMT_INCLUDE_DIR)
    endif()
endfunction()

function(set_fmt_version tgt_name vers)
    if(TARGET ${tgt_name})
        get_target_property(tgt_alias ${tgt_name} ALIASED_TARGET)
        if(tgt_alias)
            set(tgt ${tgt_alias})
        else()
            set(tgt ${tgt_name})
        endif()
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
            set_target_properties(${tgt} PROPERTIES VERSION ${${vers}})
        endif()
        if(${${vers}} VERSION_LESS 7.0.0)
            # Fix bug with ambiguous resolution of fmt::format_to which is used in spdlog 1.5.0
            target_compile_definitions(${tgt} INTERFACE FMT_USE_CONSTEXPR=0)
        endif()
    endif()
endfunction()

find_fmt()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(fmt
        FOUND_VAR fmt_FOUND
        REQUIRED_VARS FMT_INCLUDE_DIR FMT_VERSION_OK
        VERSION_VAR FMT_VERSION
        FAIL_MESSAGE "Failed to find fmt"
        )


if(fmt_FOUND)
    if(FMT_LIBRARY AND NOT TARGET fmt::fmt)
        add_library(fmt::fmt UNKNOWN IMPORTED)
        set_target_properties(fmt::fmt PROPERTIES IMPORTED_LOCATION "${FMT_LIBRARY}")
    elseif(NOT TARGET fmt::fmt)
        add_library(fmt::fmt INTERFACE IMPORTED)
    endif()
    if(NOT TARGET fmt::fmt-header-only)
        add_library(fmt::fmt-header-only INTERFACE IMPORTED)
    endif()
    target_include_directories(fmt::fmt SYSTEM INTERFACE "${FMT_INCLUDE_DIR}")
    target_include_directories(fmt::fmt-header-only SYSTEM INTERFACE "${FMT_INCLUDE_DIR}")

    target_compile_definitions(fmt::fmt-header-only INTERFACE FMT_HEADER_ONLY=1)
    target_compile_features(fmt::fmt INTERFACE cxx_variadic_templates)
    target_compile_features(fmt::fmt-header-only INTERFACE cxx_variadic_templates)

    set_fmt_version(fmt::fmt FMT_VERSION)
    set_fmt_version(fmt::fmt-header-only FMT_VERSION)
endif()