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

        set(SPDLOG_VERSION ${SPDLOG_WORLD_VERSION}.${SPDLOG_MAJOR_VERSION}.${SPDLOG_MINOR_VERSION} PARENT_SCOPE)
    endif()
endfunction()

function(find_spdlog)
    find_path(SPDLOG_INCLUDE_DIR
                NAMES spdlog/spdlog.h
                HINTS ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES spdlog/include include spdlog include/spdlog spdlog/include/spdlog
                )
    if(SPDLOG_INCLUDE_DIR)
        # There may o compiled library to go with the headers
        find_library(SPDLOG_LIBRARY
                NAMES spdlog
                HINTS ${SPDLOG_INCLUDE_DIR} ${SPDLOG_INCLUDE_DIR}../ ${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES spdlog/lib
                )
        spdlog_check_version_include(SPDLOG_INCLUDE_DIR)
    endif()

    set(SPDLOG_INCLUDE_DIR ${SPDLOG_INCLUDE_DIR} PARENT_SCOPE)
    set(SPDLOG_LIBRARY     ${SPDLOG_LIBRARY}     PARENT_SCOPE)
    set(SPDLOG_VERSION     ${SPDLOG_VERSION}     PARENT_SCOPE)
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

find_spdlog()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    set(HANDLE_VERSION_RANGE HANDLE_VERSION_RANGE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
        REQUIRED_VARS SPDLOG_INCLUDE_DIR
        VERSION_VAR SPDLOG_VERSION
        ${HANDLE_VERSION_RANGE}
        )

if(spdlog_FOUND)
    if(SPDLOG_LIBRARY AND NOT TARGET spdlog::spdlog AND NOT SPDLOG_LIBRARY MATCHES "conda")
        # Choose header-only because conda libraries sometimes give linking errors, such as:
        # /usr/bin/ld:
        #       /home/user/miniconda/lib/libspdlog.a(spdlog.cpp.o): TLS transition from R_X86_64_TLSLD
        #       to R_X86_64_TPOFF32 against `_ZGVZN6spdlog7details2os9thread_idEvE3tid'
        #       at 0x4 in section `.text._ZN6spdlog7details2os9thread_idEv' failed
        # /home/user/miniconda/lib/libspdlog.a: error adding symbols: Bad value
        add_library(spdlog::spdlog UNKNOWN IMPORTED)
        set_target_properties(spdlog::spdlog PROPERTIES IMPORTED_LOCATION "${SPDLOG_LIBRARY}")
        target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_COMPILED_LIB)
    elseif(NOT TARGET spdlog::spdlog)
        add_library(spdlog::spdlog INTERFACE IMPORTED)
        target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_HEADER_ONLY)
    endif()
    if(NOT TARGET spdlog::spdlog_header_only)
        add_library(spdlog::spdlog_header_only INTERFACE IMPORTED)
    endif()
    target_include_directories(spdlog::spdlog SYSTEM INTERFACE "${SPDLOG_INCLUDE_DIR}")
    target_include_directories(spdlog::spdlog_header_only SYSTEM INTERFACE "${SPDLOG_INCLUDE_DIR}")
    target_compile_definitions(spdlog::spdlog_header_only INTERFACE SPDLOG_HEADER_ONLY)
    set_spdlog_version(SPDLOG_VERSION)
    spdlog_link_fmt()
endif()