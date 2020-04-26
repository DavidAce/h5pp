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
# SPDLOG_DIR, spdlog_DIR, SPDLOG_ROOT spdlog_ROOT, etc.

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

macro(_spdlog_check_version)
    if(EXISTS ${SPDLOG_INCLUDE_DIR}/spdlog/version.h)
        file(READ "${SPDLOG_INCLUDE_DIR}/spdlog/version.h" _spdlog_version_header)
        string(REGEX MATCH "define[ \t]+SPDLOG_VER_MAJOR[ \t]+([0-9]+)" _spdlog_world_version_match "${_spdlog_version_header}")
        set(SPDLOG_WORLD_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+SPDLOG_VER_MINOR[ \t]+([0-9]+)" _spdlog_major_version_match "${_spdlog_version_header}")
        set(SPDLOG_MAJOR_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "define[ \t]+SPDLOG_VER_PATCH[ \t]+([0-9]+)" _spdlog_minor_version_match "${_spdlog_version_header}")
        set(SPDLOG_MINOR_VERSION "${CMAKE_MATCH_1}")

        set(SPDLOG_VERSION ${SPDLOG_WORLD_VERSION}.${SPDLOG_MAJOR_VERSION}.${SPDLOG_MINOR_VERSION})
        if(${SPDLOG_VERSION} VERSION_LESS ${spdlog_FIND_VERSION})
            set(SPDLOG_VERSION_OK FALSE)
        else()
            set(SPDLOG_VERSION_OK TRUE)
        endif()
    else()
        set(SPDLOG_VERSION_OK FALSE)
    endif()
endmacro()


if(SPDLOG_NO_DEFAULT_PATH)
    set(NO_DEFAULT_PATH NO_DEFAULT_PATH)
endif()
if(SPDLOG_NO_CMAKE_PACKAGE_REGISTRY)
    set(NO_CMAKE_PACKAGE_REGISTRY NO_CMAKE_PACKAGE_REGISTRY)
endif()


# With this particular order we can manually override where we should look for spdlog first
# Recall that H5PP_DIRECTORY_HINTS may have CONDA_PREFIX first inside if PREFER_CONDA_LIBS=ON
list(APPEND SPDLOG_DIRECTORY_HINTS
        HINTS
        ${CONAN_SPDLOG_ROOT}
        $ENV{EBROOTSPDLOG}
        ${H5PP_DIRECTORY_HINTS}
        ${CMAKE_INSTALL_PREFIX}
        ${CMAKE_BINARY_DIR}/h5pp-deps-install
        ${CMAKE_INSTALL_PREFIX}/include
        )

# First try finding a config somewhere in the system
if(NOT SPDLOG_NO_CONFIG OR SPDLOG_CONFIG_ONLY)
    find_package(spdlog ${spdlog_FIND_VERSION}
            HINTS ${SPDLOG_DIRECTORY_HINTS}
            PATHS $ENV{CONDA_PREFIX}
            PATH_SUFFIXES include spdlog include/spdlog spdlog/include/spdlog
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_PACKAGE_REGISTRY}
            )

    if(TARGET spdlog::spdlog)
        _spdlog_check_version()
        if(NOT SPDLOG_VERSION_OK OR NOT SPDLOG_VERSION)
          message(WARNING "Could not determine the spdlog version.\n"
                  "However, the target spdlog::spdlog has already been defined, so it will be used.\n"
                  "Something is wrong with your installation of spdlog")
        endif()
        target_compile_definitions(spdlog::spdlog INTERFACE H5PP_SPDLOG)
        get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
        if(SPDLOG_INCLUDE_DIR MATCHES "conda")
            # Use the header-only mode to avoid weird linking errors
            set_target_properties(spdlog::spdlog PROPERTIES INTERFACE_COMPILE_DEFINITIONS "SPDLOG_HEADER_ONLY")
        endif()
        find_library(FMT_LIBRARY
                NAMES fmt
                HINTS ${SPDLOG_DIRECTORY_HINTS}
                PATHS $ENV{CONDA_PREFIX}
                PATH_SUFFIXES spdlog/${CMAKE_INSTALL_LIBDIR}  ${CMAKE_INSTALL_LIBDIR}
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_PACKAGE_REGISTRY}
                )
        if(FMT_LIBRARY AND NOT FMT_LIBRARY MATCHES "conda")
            target_link_libraries(spdlog::spdlog INTERFACE ${FMT_LIBRARY} )
        else()
            target_compile_definitions(spdlog::spdlog INTERFACE FMT_HEADER_ONLY )
        endif()
    endif()
endif()

if(NOT TARGET spdlog::spdlog AND NOT TARGET spdlog AND NOT SPDLOG_CONFIG_ONLY)
    find_path(SPDLOG_INCLUDE_DIR
            NAMES spdlog/spdlog.h
            HINTS ${SPDLOG_DIRECTORY_HINTS}
            PATHS $ENV{CONDA_PREFIX}
            PATH_SUFFIXES spdlog/include include spdlog include/spdlog spdlog/include/spdlog
            ${NO_DEFAULT_PATH}
            ${NO_CMAKE_PACKAGE_REGISTRY}
            QUIET
            )

    if(SPDLOG_INCLUDE_DIR)
        _spdlog_check_version()
        if(SPDLOG_VERSION_OK)
            set(spdlog_FOUND TRUE)
            add_library(spdlog INTERFACE)
            target_include_directories(spdlog INTERFACE ${SPDLOG_INCLUDE_DIR})
            target_compile_definitions(spdlog INTERFACE H5PP_SPDLOG)
            find_path(SPDLOG_FMT_BUNDLED
                    spdlog/fmt/fmt.h
                    HINTS ${SPDLOG_INCLUDE_DIR} ${SPDLOG_DIRECTORY_HINTS}
                    PATHS $ENV{CONDA_PREFIX}
                    PATH_SUFFIXES spdlog/include include spdlog include/spdlog spdlog/include/spdlog
                    ${NO_DEFAULT_PATH}
                    ${NO_CMAKE_PACKAGE_REGISTRY}
                    )
            include(GNUInstallDirs)
            find_library(SPDLOG_LIBRARY
                    NAMES spdlog
                    HINTS ${SPDLOG_INCLUDE_DIR} ${SPDLOG_DIRECTORY_HINTS}
                    PATHS $ENV{CONDA_PREFIX}
                    PATH_SUFFIXES spdlog/${CMAKE_INSTALL_LIBDIR}  ${CMAKE_INSTALL_LIBDIR}
                    ${NO_DEFAULT_PATH}
                    ${NO_CMAKE_PACKAGE_REGISTRY}
                    )

            find_library(FMT_LIBRARY
                    NAMES fmt
                    HINTS ${SPDLOG_INCLUDE_DIR} ${SPDLOG_DIRECTORY_HINTS}
                    PATHS $ENV{CONDA_PREFIX}
                    PATH_SUFFIXES spdlog/${CMAKE_INSTALL_LIBDIR}  ${CMAKE_INSTALL_LIBDIR}
                    ${NO_DEFAULT_PATH}
                    ${NO_CMAKE_PACKAGE_REGISTRY}
                    )
            if(NOT SPDLOG_FMT_BUNDLED)
                target_compile_definitions(spdlog INTERFACE SPDLOG_FMT_EXTERNAL)
            endif()
            if(SPDLOG_LIBRARY AND NOT SPDLOG_LIBRARY MATCHES "conda")
                # Conda libraries sometimes give weird linking errors, such as:
                # /usr/bin/ld:
                #       /home/user/miniconda/lib/libspdlog.a(spdlog.cpp.o): TLS transition from R_X86_64_TLSLD
                #       to R_X86_64_TPOFF32 against `_ZGVZN6spdlog7details2os9thread_idEvE3tid'
                #       at 0x4 in section `.text._ZN6spdlog7details2os9thread_idEv' failed
                # /home/user/miniconda/lib/libspdlog.a: error adding symbols: Bad value

                target_link_libraries(spdlog INTERFACE ${SPDLOG_LIBRARY} )
                target_compile_definitions(spdlog INTERFACE SPDLOG_COMPILED_LIB )
            else()
                target_compile_definitions(spdlog INTERFACE SPDLOG_HEADER_ONLY)
            endif()
            if(FMT_LIBRARY AND NOT FMT_LIBRARY MATCHES "conda")
                target_link_libraries(spdlog INTERFACE ${FMT_LIBRARY})
            else()
                target_compile_definitions(spdlog INTERFACE FMT_HEADER_ONLY )
            endif()
        endif()
    endif()
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