cmake_minimum_required(VERSION 3.24)

function(pkg_message lvl msg)
    message(${lvl} "PkgInstall [${pkg_name}]: ${msg} ${ARGN}")
endfunction()

macro(pkg_set var val)
    set(${var} ${val} ${ARGN})
    set(${var} ${val} ${ARGN} PARENT_SCOPE)
    mark_as_advanced(${var})
endmacro()

# Dumps cached variables to PKG_INIT_CACHE_FILE so that we can propagate
# the current build configuration to dependencies
function(generate_init_cache)
    set(PKG_INIT_CACHE_FILE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/init-cache.cmake)
    set(PKG_INIT_CACHE_FILE ${PKG_INIT_CACHE_FILE} PARENT_SCOPE)
    file(WRITE  ${PKG_INIT_CACHE_FILE} "# These variables will initialize the CMake cache for subprocesses.\n")
    get_cmake_property(vars CACHE_VARIABLES)
    foreach(var ${vars})
        if(var MATCHES "CMAKE_CACHE|CMAKE_HOME|CMAKE_EXTRA|CMAKE_PROJECT|MACRO")
            continue()
        endif()
        get_property(help CACHE "${var}" PROPERTY HELPSTRING)
        get_property(type CACHE "${var}" PROPERTY TYPE)
        string(REPLACE "\\" "/" ${var} "${${var}}") # Fix windows backslash paths
        string(REPLACE "\"" "\\\"" help "${help}") #Fix quotes on some cuda-related descriptions
        string(REPLACE "\"" "\\\"" ${var} "${${var}}") #Fix quotes
        file(APPEND ${PKG_INIT_CACHE_FILE} "set(${var} \"${${var}}\" CACHE ${type} \"${help}\" FORCE)\n")
    endforeach()
endfunction()


# This function will configure, build and install a package at configure-time
# by running cmake in a subprocess. The current CMake configuration is transmitted
# by setting the flags manually.
function(pkg_install pkg_name)
    set(options DEBUG INSTALL_PREFIX_PKGNAME)
    set(oneValueArgs VERSION INSTALL_DIR INSTALL_SUBDIR BUILD_DIR BUILD_SUBDIR LINK_TYPE)
    set(multiValueArgs HINTS PATHS PATH_SUFFIXES COMPONENTS CMAKE_ARGS LIBRARY_NAMES TARGET_HINTS)
    cmake_parse_arguments(PARSE_ARGV 1 PKG "${options}" "${oneValueArgs}" "${multiValueArgs}")


    # Set directories
    if(NOT PKG_BUILD_DIR)
        set(PKG_BUILD_DIR ${CMAKE_BINARY_DIR}/pkg-build)
    endif()

    if(NOT PKG_INSTALL_DIR)
        if(NOT CMAKE_INSTALL_PREFIX)
            set(PKG_INSTALL_DIR ${CMAKE_BINARY_DIR}/pkg-install)
        else()
            set(PKG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
        endif()
    endif()

    # Append to CMAKE_PREFIX_PATH so we can find the packages later
    pkg_message(DEBUG "Appending to CMAKE_PREFIX_PATH: ${PKG_INSTALL_DIR}")
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PKG_INSTALL_DIR})
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE INTERNAL "Paths for find_package lookup" FORCE)


    if(PKG_INSTALL_SUCCESS_${pkg_name})
        pkg_message(DEBUG "Already installed: ${pkg_name} (detected cache variable: PKG_INSTALL_SUCCESS_${pkg_name})")
        return()
    endif()
    if(EXISTS "${PKG_INSTALL_DIR}/.pkg_install_success_${pkg_name}")
       pkg_message(DEBUG "Already installed: ${pkg_name} (found manifest: ${PKG_INSTALL_DIR}/.pkg_install_success_${pkg_name})")
       return()
    endif()

    if(DEFINED ENV{HOME})
        file(LOCK $ENV{HOME}/cmake.${PROJECT_NAME}.lock GUARD FUNCTION TIMEOUT 600)
    elseif(DEFINED ENV{USERPROFILE})
        file(LOCK $ENV{USERPROFILE}/cmake.${PROJECT_NAME}.lock GUARD FUNCTION TIMEOUT 600)
    endif()

    # Append to CMAKE_PREFIX_PATH so we can find the packages later
    pkg_message(DEBUG "Appending to CMAKE_PREFIX_PATH: ${PKG_INSTALL_DIR}")
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PKG_INSTALL_DIR})
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE INTERNAL "Paths for find_package lookup" FORCE)

    pkg_message(STATUS "${pkg_name} will be installed into ${PKG_INSTALL_DIR}")

    # Generate an init cache to propagate the current configuration
    generate_init_cache()

    # Configure the package
    execute_process(COMMAND  ${CMAKE_COMMAND} -E make_directory ${PKG_BUILD_DIR})
    execute_process(COMMAND  ${CMAKE_COMMAND} -E remove ${PKG_BUILD_DIR}/CMakeCache.txt)
    execute_process(
            COMMAND
            ${CMAKE_COMMAND}
            -C ${PKG_INIT_CACHE_FILE}                # For the subproject in external_<libname>
            -DINIT_CACHE_FILE=${PKG_INIT_CACHE_FILE} # For externalproject_add inside the subproject
            -DCMAKE_INSTALL_PREFIX:PATH=${PKG_INSTALL_DIR}
            -DCMAKE_POLICY_DEFAULT_CMP0074:STRING=NEW # "Honor <PackageName>_ROOT"
            -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW # "Use MSVC_RUNTIME_LIBRARY", fixes spdlog on MSVC
            -DCMAKE_POLICY_DEFAULT_CMP0135:STRING=NEW # "Use DOWNLOAD_EXTRACT_TIMESTAMP"
            -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=TRUE # "Otherwise fails with -G Ninja"
            ${PKG_CMAKE_ARGS}
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external_${pkg_name}
            WORKING_DIRECTORY ${PKG_BUILD_DIR}
            RESULT_VARIABLE config_result
    )
    if(config_result)
        pkg_message(STATUS "Got non-zero exit code while configuring ${pkg_name}")
        pkg_message(STATUS "build_dir             : ${PKG_BUILD_DIR}")
        pkg_message(STATUS "install_dir           : ${PKG_INSTALL_DIR}")
        pkg_message(STATUS "extra_flags           : ${extra_flags}")
        pkg_message(STATUS "config_result         : ${config_result}")
        pkg_message(FATAL_ERROR "Failed to configure ${pkg_name}")
    endif()


    # Make sure to do multithreaded builds if possible
    cmake_host_system_information(RESULT num_threads QUERY NUMBER_OF_PHYSICAL_CORES)


    # Build the package
    if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        # This branch is for multi-config generators such as Visual Studio 16 2019
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads} --config ${config}
                    WORKING_DIRECTORY "${PKG_BUILD_DIR}"
                    RESULT_VARIABLE build_result
                    )
            if(build_result)
                pkg_message(STATUS "Got non-zero exit code while building package: ${pkg_name}")
                pkg_message(STATUS  "build config      : ${config}")
                pkg_message(STATUS  "build dir         : ${PKG_BUILD_DIR}")
                pkg_message(STATUS  "install dir       : ${PKG_INSTALL_DIR}")
                pkg_message(STATUS  "cmake args        : ${PKG_CMAKE_ARGS}")
                pkg_message(STATUS  "build result      : ${build_result}")
                pkg_message(FATAL_ERROR "Failed to build package: ${pkg_name}")
            endif()
        endforeach()
    else()
        # This is for single-config generators such as Unix Makefiles and Ninja
        execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads}
                WORKING_DIRECTORY "${PKG_BUILD_DIR}"
                RESULT_VARIABLE build_result
                )
        if(build_result)
            pkg_message(STATUS "Got non-zero exit code while building package: ${pkg_name}")
            pkg_message(STATUS  "build dir         : ${PKG_BUILD_DIR}")
            pkg_message(STATUS  "install dir       : ${PKG_INSTALL_DIR}")
            pkg_message(STATUS  "cmake args        : ${PKG_CMAKE_ARGS}")
            pkg_message(STATUS  "build result      : ${build_result}")
            pkg_message(FATAL_ERROR "Failed to build package: ${pkg_name}")
        endif()
    endif()


    # Copy the install manifest if it exists
    file(GLOB_RECURSE INSTALL_MANIFEST "${PKG_BUILD_DIR}/${pkg_name}/install_manifest*.txt")
    foreach(manifest ${INSTALL_MANIFEST})
        get_filename_component(manifest_filename ${manifest} NAME_WE)
        message(STATUS "Copying install manifest: ${manifest}")
        configure_file(${manifest} ${CMAKE_BINARY_DIR}/${manifest_filename}_${pkg_name}.txt)
    endforeach()
    file(TOUCH ${PKG_INSTALL_DIR}/.pkg_install_success_${pkg_name})
    set(PKG_INSTALL_SUCCESS_${pkg_name} TRUE CACHE BOOL "PKG installed ${pkg_name} successfully in ${PKG_INSTALL_DIR}")
endfunction()

