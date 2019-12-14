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

    #define SPDLOG_VER_MAJOR 1
    #define SPDLOG_VER_MINOR 4
    #define SPDLOG_VER_PATCH 2

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
    if(NOT SPDLOG_VERSION_OK)
        message(STATUS "spdlog version ${SPDLOG_VERSION} found in ${SPDLOG_INCLUDE_DIR}, "
                "but at least version ${spdlog_FIND_VERSION} is required")
    endif()
endmacro()


if(SPDLOG_NO_DEFAULT_PATH)
    set(NO_DEFAULT_PATH NO_DEFAULT_PATH)
endif()
if(SPDLOG_NO_CMAKE_PACKAGE_REGISTRY)
    set(NO_CMAKE_PACKAGE_REGISTRY NO_CMAKE_PACKAGE_REGISTRY)
endif()



# First try finding a config somewhere in the system
if(NOT SPDLOG_NO_CONFIG OR SPDLOG_CONFIG_ONLY)
    find_package(spdlog ${spdlog_FIND_VERSION}
            HINTS
                ${spdlog_ROOT} $ENV{spdlog_ROOT}
                ${SPDLOG_ROOT} $ENV{SPDLOG_ROOT}
                ${H5PP_DIRECTORY_HINTS}
                ${CMAKE_INSTALL_PREFIX}
                ${CMAKE_INSTALL_PREFIX}/include
                ${CMAKE_BINARY_DIR}/h5pp-deps-install
            PATHS
                $ENV{EBROOTSPDLOG} $ENV{CONDA_PREFIX}
            PATH_SUFFIXES include spdlog include/spdlog spdlog/include/spdlog
                ${NO_DEFAULT_PATH}
                ${NO_CMAKE_PACKAGE_REGISTRY}
            )
    if(TARGET spdlog::spdlog)
        get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
        _spdlog_check_version()
    endif()
endif()

if(NOT TARGET spdlog::spdlog AND NOT TARGET spdlog AND NOT SPDLOG_CONFIG_ONLY)
    find_path(SPDLOG_INCLUDE_DIR
            NAMES spdlog/spdlog.h
            HINTS
                ${spdlog_ROOT} $ENV{spdlog_ROOT}
                ${SPDLOG_ROOT} $ENV{SPDLOG_ROOT}
                ${spdlog_DIR} ${SPDLOG_DIR}
                ${H5PP_DIRECTORY_HINTS}
                ${CMAKE_INSTALL_PREFIX}
                ${CMAKE_INSTALL_PREFIX}/include
                ${CMAKE_BINARY_DIR}/h5pp-deps-install
            PATHS
                $ENV{EBROOTSPDLOG} $ENV{CONDA_PREFIX}
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
            find_path(SPDLOG_FMT_BUNDLED
                    spdlog/fmt/bundled/core.h
                    HINTS
                        ${SPDLOG_INCLUDE_DIR}
                        ${spdlog_ROOT} $ENV{spdlog_ROOT}
                        ${SPDLOG_ROOT} $ENV{SPDLOG_ROOT}
                        ${spdlog_DIR} ${SPDLOG_DIR}
                        ${H5PP_DIRECTORY_HINTS}
                        ${CMAKE_INSTALL_PREFIX}
                        ${CMAKE_INSTALL_PREFIX}/include
                        ${CMAKE_BINARY_DIR}/h5pp-deps-install
                    PATHS
                        $ENV{EBROOTSPDLOG} $ENV{CONDA_PREFIX}
                    PATH_SUFFIXES spdlog/include include spdlog include/spdlog spdlog/include/spdlog
                    ${NO_DEFAULT_PATH}
                    ${NO_CMAKE_PACKAGE_REGISTRY}
                    )
            include(GNUInstallDirs)
            find_library(SPDLOG_LIBRARY
                    NAMES spdlog
                    HINTS
                        ${SPDLOG_INCLUDE_DIR}
                        ${spdlog_ROOT} $ENV{spdlog_ROOT}
                        ${SPDLOG_ROOT} $ENV{SPDLOG_ROOT}
                        ${spdlog_DIR} ${SPDLOG_DIR}
                        ${H5PP_DIRECTORY_HINTS}
                        ${CMAKE_INSTALL_PREFIX}
                        ${CMAKE_INSTALL_PREFIX}/include
                        ${CMAKE_BINARY_DIR}/h5pp-deps-install
                    PATHS
                        $ENV{EBROOTSPDLOG} $ENV{CONDA_PREFIX}
                    PATH_SUFFIXES spdlog/${CMAKE_INSTALL_LIBDIR}  ${CMAKE_INSTALL_LIBDIR}
                    ${NO_DEFAULT_PATH}
                    ${NO_CMAKE_PACKAGE_REGISTRY}
                    )

            if(NOT SPDLOG_FMT_BUNDLED)
                target_compile_definitions(spdlog INTERFACE SPDLOG_FMT_EXTERNAL)
            endif()
            if(SPDLOG_LIBRARY)
                target_link_libraries(spdlog INTERFACE ${SPDLOG_LIBRARY} )
                target_compile_definitions(spdlog INTERFACE SPDLOG_COMPILED_LIB )
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