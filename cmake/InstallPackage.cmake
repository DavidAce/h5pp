cmake_minimum_required(VERSION 3.15)
include(cmake/CheckCompile.cmake)

function(pkg_message lvl msg)
    message(${lvl} "InstallPackage[${pkg_find_name}]: ${msg} ${ARGN}")
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
function(install_package pkg_name)
    if(DEFINED ENV{HOME})
        file(LOCK $ENV{HOME}/cmake.${PROJECT_NAME}.lock GUARD FUNCTION TIMEOUT 600)
    elseif(DEFINED ENV{USERPROFILE})
        file(LOCK $ENV{USERPROFILE}/cmake.${PROJECT_NAME}.lock GUARD FUNCTION TIMEOUT 600)
    endif()
    set(options CONFIG MODULE CHECK QUIET DEBUG INSTALL_PREFIX_PKGNAME)
    set(oneValueArgs VERSION INSTALL_DIR INSTALL_SUBDIR BUILD_DIR BUILD_SUBDIR FIND_NAME TARGET_NAME LINK_TYPE)
    set(multiValueArgs HINTS PATHS PATH_SUFFIXES COMPONENTS DEPENDS CMAKE_ARGS LIBRARY_NAMES TARGET_HINTS)
    cmake_parse_arguments(PARSE_ARGV 1 PKG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # Set defaults
    if(NOT PKG_BUILD_DIR)
        if(PKG_BUILD_DIR_DEFAULT)
            set(PKG_BUILD_DIR ${PKG_BUILD_DIR_DEFAULT})
        else()
            set(PKG_BUILD_DIR ${CMAKE_BINARY_DIR}/pkg-build)
        endif()
    endif()
    if(NOT PKG_INSTALL_DIR)
        if(PKG_INSTALL_DIR_DEFAULT)
            set(PKG_INSTALL_DIR ${PKG_INSTALL_DIR_DEFAULT})
        else()
            set(PKG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
        endif()
    endif()
    if(IS_ABSOLUTE PKG_BUILD_SUBDIR)
        pkg_message(FATAL_ERROR "PKG_BUILD_SUBDIR must be a relative path: ${PKG_BUILD_SUBDIR}")
    endif()
    if(IS_ABSOLUTE PKG_INSTALL_SUBDIR)
        pkg_message(FATAL_ERROR "PKG_INSTALL_SUBDIR must be a relative path: ${PKG_INSTALL_SUBDIR}")
    endif()

    # Further parsing / override defaults
    if(PKG_BUILD_SUBDIR)
        string(JOIN / pkg_build_dir ${PKG_BUILD_DIR} ${PKG_BUILD_SUBDIR})
    else()
        string(JOIN / pkg_build_dir ${PKG_BUILD_DIR} ${pkg_name})
    endif()

    if(PKG_INSTALL_SUBDIR)
        string(JOIN / pkg_install_dir ${PKG_INSTALL_DIR} ${PKG_INSTALL_SUBDIR})
    elseif(PKG_INSTALL_PREFIX_PKGNAME)
        string(JOIN / pkg_install_dir ${PKG_INSTALL_DIR} ${pkg_name})
    else()
        set(pkg_install_dir ${PKG_INSTALL_DIR})
    endif()

    set(pkg_target_name ${pkg_name}::${pkg_name})
    set(pkg_find_name   ${pkg_name})

    if(PKG_FIND_NAME)
        set(pkg_find_name ${PKG_FIND_NAME})
    endif()
    if(PKG_TARGET_NAME)
        set(pkg_target_name ${PKG_TARGET_NAME})
        list(APPEND PKG_TARGET_HINTS ${pkg_target_name})
    endif()
    if(PKG_CONFIG)
        set(CONFIG CONFIG)
    endif()
    if(PKG_QUIET)
        set(QUIET QUIET)
    endif()
    if(PKG_COMPONENTS)
        set(COMPONENTS COMPONENTS)
    endif()
    if(PKG_LINK_TYPE)
        set(pkg_link_type ${PKG_LINK_TYPE})
    elseif(BUILD_SHARED_LIBS)
        set(pkg_link_type shared)
    else()
        set(pkg_link_type static)
    endif()

    
    if(NOT PKG_TARGET_NAME)
        foreach(tgt ${PKG_TARGET_HINTS})
            list(APPEND PKG_TARGET_HINTS_LINK_TYPE ${tgt}-${pkg_link_type})
        endforeach()

        list(APPEND PKG_TARGET_HINTS
                ${pkg_target_name}
                ${pkg_name}::${pkg_name}
                ${pkg_find_name}::${pkg_find_name}
                ${pkg_name}
                ${pkg_find_name}
                ${pkg_name}::${pkg_name}-${pkg_link_type}
                ${pkg_find_name}::${pkg_find_name}-${pkg_link_type}
                ${pkg_name}-${pkg_link_type}
                ${pkg_find_name}-${pkg_link_type}
                ${PKG_TARGET_HINTS_LINK_TYPE}
                )
    endif()
    list(REMOVE_DUPLICATES PKG_TARGET_HINTS)

    foreach (tgt ${PKG_DEPENDS})
        if(NOT TARGET ${tgt})
            list(APPEND PKG_MISSING_TARGET ${tgt})
        endif()
    endforeach()
    if(PKG_MISSING_TARGET)
        pkg_message(FATAL_ERROR "Could not install ${pkg_name}: dependencies missing [${PKG_MISSING_TARGET}]")
    endif()

    pkg_message(VERBOSE "Starting install")

    # Append to CMAKE_PREFIX_PATH so we can find the packages later
    pkg_message(DEBUG "Appending to CMAKE_PREFIX_PATH: ${pkg_install_dir}")
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${pkg_install_dir} ${PKG_INSTALL_DIR})
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE INTERNAL "Paths for find_package lookup" FORCE)


    # Try finding config files before modules
    pkg_message(DEBUG "Prefer CONFIG mode ON")
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

    if(PKG_LIBRARY_NAMES)
        # This attempts to find <PackageName>_LIBRARY with given names before calling find_package
        # This is necessary for ZLIB
        if(pkg_link_type MATCHES "static|STATIC|Static")
            set(pkg_link_type_msg ${pkg_link_type})
            set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
        endif()
        pkg_message(DEBUG "Looking for ${pkg_link_type_msg} library names ${PKG_LIBRARY_NAMES}")
        find_library(${pkg_find_name}_LIBRARY
                NAMES ${PKG_LIBRARY_NAMES}
                HINTS ${pkg_install_dir} ${${pkg_find_name}_ROOT} ${${pkg_name}_ROOT}
                PATH_SUFFIXES ${pkg_name} ${pkg_find_name} lib ${pkg_find_name}/lib ${pkg_name}/lib
                NO_CMAKE_ENVIRONMENT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                )
        if(${pkg_find_name}_LIBRARY)
            pkg_message(VERBOSE "Found ${pkg_find_name}_LIBRARY: ${${pkg_find_name}_LIBRARY}")
            find_package(${pkg_find_name} ${PKG_VERSION} ${COMPONENTS} ${PKG_COMPONENTS} ${QUIET})
        endif()
    elseif(PKG_MODULE)
        pkg_message(DEBUG "Looking for package in MODULE mode")
        find_package(${pkg_find_name} ${PKG_VERSION} ${COMPONENTS} ${PKG_COMPONENTS} ${QUIET})
    else()
        pkg_message(DEBUG "Looking for package in CONFIG mode")
        find_package(${pkg_find_name} ${PKG_VERSION}
                HINTS ${PKG_HINTS}
                PATHS ${PKG_PATHS}
                PATH_SUFFIXES ${PKG_PATH_SUFFIXES}
                ${COMPONENTS} ${PKG_COMPONENTS}
                ${CONFIG} ${QUIET}
                # These lets us ignore system packages
                NO_SYSTEM_ENVIRONMENT_PATH #5
                NO_CMAKE_PACKAGE_REGISTRY #6
                NO_CMAKE_SYSTEM_PATH #7
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY #8
                )
    endif()
    # Set _FOUND variables for alternate names and components
    if(NOT ${pkg_name}_FOUND)
        if(${pkg_find_name}_FOUND)
            pkg_set(${pkg_name}_FOUND TRUE)
        else()
            # Some packages, such as SZIP, may only set each components as found but not the package itself,
            # with variables like SZIP_shared_FOUND and so on
            foreach(comp ${COMPONENTS})
                if(${pkg_name}_${comp}_FOUND)
                    pkg_set(${pkg_name}_FOUND TRUE)
                endif()
            endforeach()
        endif()
    endif()
    # Check if the package was found
    if(${pkg_name}_FOUND)
        foreach(tgt ${PKG_TARGET_HINTS})
            if(TARGET ${tgt})
                pkg_set(${pkg_name}_FOUND TRUE)
                pkg_set(${pkg_find_name}_FOUND TRUE)
                pkg_set(PKG_${pkg_name}_TARGET ${tgt})
                pkg_set(PKG_${pkg_name}_FOUND TRUE)
                pkg_set(PKG_${pkg_find_name}_TARGET ${tgt})
                pkg_set(PKG_${pkg_find_name}_FOUND TRUE)
                set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS};${tgt})
                list(REMOVE_DUPLICATES PKG_INSTALLED_TARGETS)
                set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS} CACHE INTERNAL "")
                break()
            endif()
        endforeach()
        if(TARGET ${PKG_${pkg_name}_TARGET})
            pkg_message(VERBOSE "Found ${pkg_name}: [${PKG_${pkg_name}_TARGET}]")
        else()
            pkg_message(WARNING "Found ${pkg_name} but no target matches the hint list: [${PKG_TARGET_HINTS}]")
        endif()
        if(PKG_DEPENDS)
            target_link_libraries(${PKG_${pkg_name}_TARGET} INTERFACE ${PKG_DEPENDS})
        endif()
        if(PKG_CHECK)
            check_compile(${pkg_name} ${PKG_${pkg_name}_TARGET} ${PROJECT_SOURCE_DIR}/cmake/compile/${pkg_name}.cpp)
            if(PKG_DEBUG AND NOT check_compile_${pkg_name} AND EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log")
                file(READ "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log" ERROR_LOG)
                pkg_message(STATUS "CMakeError.log: \n ${ERROR_LOG}")
            endif()
        endif()
        return()
    endif()

    pkg_message(STATUS "${pkg_name} will be installed into ${pkg_install_dir}")

    set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
    set(CMAKE_CXX_STANDARD_REQUIRED TRUE CACHE BOOL "")
    set(CMAKE_CXX_EXTENSIONS FALSE CACHE BOOL "")

    # Set policies for CMakeLists in packages that require older CMake versions
    set(CMAKE_POLICY_DEFAULT_CMP0074 NEW CACHE STRING "Honor <PackageName>_ROOT")
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW CACHE STRING "Use MSVC_RUNTIME_LIBRARY") # Fixes spdlog on MSVC

    if(CMAKE_GENERATOR MATCHES "Ninja")
        set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE CACHE BOOL "Otherwise fails with -G Ninja" FORCE)
    endif()

    # Generate an init cache to propagate the current configuration
    generate_init_cache()

    # Configure the package
    execute_process( COMMAND  ${CMAKE_COMMAND} -E make_directory ${pkg_build_dir})
    execute_process( COMMAND  ${CMAKE_COMMAND} -E remove ${pkg_build_dir}/CMakeCache.txt)
    execute_process(
            COMMAND
            ${CMAKE_COMMAND}
            -C ${PKG_INIT_CACHE_FILE}                # For the subproject in external_<libname>
            -DINIT_CACHE_FILE=${PKG_INIT_CACHE_FILE} # For externalproject_add inside the subproject
            -DCMAKE_INSTALL_PREFIX:PATH=${pkg_install_dir}
            ${PKG_CMAKE_ARGS}
            ${PROJECT_SOURCE_DIR}/cmake/external_${pkg_name}
            WORKING_DIRECTORY ${pkg_build_dir}
            RESULT_VARIABLE config_result
    )
    if(config_result)
        pkg_message(STATUS "Got non-zero exit code while configuring ${pkg_name}")
        pkg_message(STATUS "build_dir             : ${pkg_build_dir}")
        pkg_message(STATUS "install_dir           : ${pkg_install_dir}")
        pkg_message(STATUS "extra_flags           : ${extra_flags}")
        pkg_message(STATUS "config_result         : ${config_result}")
        pkg_message(FATAL_ERROR "Failed to configure ${pkg_name}")
    endif()


    # Make sure to do multithreaded builds if possible
    include(cmake/GetNumThreads.cmake)
    get_num_threads(num_threads)

    # Build the package
    if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        # This branch is for multi-config generators such as Visual Studio 16 2019
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads} --config ${config}
                    WORKING_DIRECTORY "${pkg_build_dir}"
                    RESULT_VARIABLE build_result
                    )
            if(build_result)
                pkg_message(STATUS "Got non-zero exit code while building package: ${pkg_name}")
                pkg_message(STATUS  "build config      : ${config}")
                pkg_message(STATUS  "build dir         : ${pkg_build_dir}")
                pkg_message(STATUS  "install dir       : ${pkg_install_dir}")
                pkg_message(STATUS  "cmake args        : ${PKG_CMAKE_ARGS}")
                pkg_message(STATUS  "build result      : ${build_result}")
                pkg_message(FATAL_ERROR "Failed to build package: ${pkg_name}")
            endif()
        endforeach()
    else()
        # This is for single-config generators such as Unix Makefiles and Ninja
        execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads}
                WORKING_DIRECTORY "${pkg_build_dir}"
                RESULT_VARIABLE build_result
                )
        if(build_result)
            pkg_message(STATUS "Got non-zero exit code while building package: ${pkg_name}")
            pkg_message(STATUS  "build dir         : ${pkg_build_dir}")
            pkg_message(STATUS  "install dir       : ${pkg_install_dir}")
            pkg_message(STATUS  "cmake args        : ${PKG_CMAKE_ARGS}")
            pkg_message(STATUS  "build result      : ${build_result}")
            pkg_message(FATAL_ERROR "Failed to build package: ${pkg_name}")
        endif()
    endif()


    # Copy the install manifest if it exists
    file(GLOB_RECURSE INSTALL_MANIFEST "${pkg_build_dir}/*/install_manifest*.txt")
    foreach(manifest ${INSTALL_MANIFEST})
        get_filename_component(manifest_filename ${manifest} NAME_WE)
        message(STATUS "Copying install manifest: ${manifest}")
        configure_file(${manifest} ${CMAKE_CURRENT_BINARY_DIR}/${manifest_filename}_${dep_name}.txt)
    endforeach()


    # Find the package again
    if(PKG_LIBRARY_NAMES)
        # This attempts to find <PackageName>_LIBRARY with given names before calling find_package
        # This is necessary for ZLIB
        find_library(${pkg_find_name}_LIBRARY
                NAMES ${PKG_LIBRARY_NAMES}
                HINTS ${pkg_install_dir} ${${pkg_find_name}_ROOT} ${${pkg_name}_ROOT}
                PATH_SUFFIXES ${pkg_name} ${pkg_find_name} lib ${pkg_find_name}/lib ${pkg_name}/lib
                NO_DEFAULT_PATH REQUIRED
                )
        find_package(${pkg_find_name} ${PKG_VERSION} ${COMPONENTS} ${PKG_COMPONENTS} ${QUIET} REQUIRED)
    endif()
    if(PKG_MODULE)
        find_package(${pkg_find_name} ${PKG_VERSION} ${COMPONENTS} ${PKG_COMPONENTS} ${QUIET} REQUIRED)
    else()
        find_package(${pkg_find_name} ${PKG_VERSION}
                HINTS ${pkg_install_dir}
                PATH_SUFFIXES ${PKG_PATH_SUFFIXES}
                ${COMPONENTS} ${PKG_COMPONENTS}
                ${CONFIG} ${QUIET}
                NO_DEFAULT_PATH REQUIRED)
    endif()
    foreach(tgt ${PKG_TARGET_HINTS})
        if(TARGET ${tgt})
            pkg_set(${pkg_name}_FOUND TRUE)
            pkg_set(${pkg_find_name}_FOUND TRUE)
            pkg_set(PKG_${pkg_name}_TARGET ${tgt})
            pkg_set(PKG_${pkg_name}_FOUND TRUE)
            pkg_set(PKG_${pkg_find_name}_TARGET ${tgt})
            pkg_set(PKG_${pkg_find_name}_FOUND TRUE)
            set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS};${tgt})
            list(REMOVE_DUPLICATES PKG_INSTALLED_TARGETS)
            set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS} CACHE INTERNAL "")
            break()
        endif()
    endforeach()
    if(TARGET ${PKG_${pkg_name}_TARGET})
        pkg_message(VERBOSE "Found ${pkg_name}: [${PKG_${pkg_name}_TARGET}]")
    else()
        pkg_message(WARNING "Found ${pkg_name} but no target matches hint list:\n [${PKG_TARGET_HINTS}]")
    endif()
    if(PKG_DEPENDS)
        target_link_libraries(${PKG_${pkg_name}_TARGET} INTERFACE ${PKG_DEPENDS})
    endif()
    if(PKG_CHECK)
        check_compile(${pkg_name} ${PKG_${pkg_name}_TARGET} ${PROJECT_SOURCE_DIR}/cmake/compile/${pkg_name}.cpp)
        if(PKG_DEBUG AND NOT check_compile_${pkg_name} AND EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log")
            file(READ "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log" ERROR_LOG)
            pkg_message(STATUS "CMakeError.log: \n ${ERROR_LOG}")
        endif()
    endif()

endfunction()

