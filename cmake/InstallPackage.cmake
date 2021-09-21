cmake_minimum_required(VERSION 3.15)
include(cmake/CheckCompile.cmake)

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
    file(LOCK $ENV{HOME}/cmake.${PROJECT_NAME}.lock GUARD FUNCTION TIMEOUT 600)

    set(options CONFIG MODULE CHECK QUIET DEBUG INSTALL_PREFIX_PKGNAME)
    set(oneValueArgs VERSION INSTALL_DIR BUILD_DIR FIND_NAME TARGET_NAME)
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

    # Further parsing / override defaults
    set(pkg_build_dir   ${PKG_BUILD_DIR}/${pkg_name})
    set(pkg_install_dir ${PKG_INSTALL_DIR})
    set(pkg_target_name ${pkg_name}::${pkg_name})
    set(pkg_find_name   ${pkg_name})
    if(PKG_INSTALL_PREFIX_PKGNAME)
        set(pkg_install_dir ${pkg_install_dir}/${pkg_name})
    endif()
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
    if(BUILD_SHARED_LIBS)
        set(pkg_link shared)
    else()
        set(pkg_link static)
    endif()
    if(NOT PKG_TARGET_NAME)
        list(APPEND PKG_TARGET_HINTS
                ${pkg_target_name}
                ${pkg_name}::${pkg_name}
                ${pkg_find_name}::${pkg_find_name}
                ${pkg_name}
                ${pkg_find_name}
                ${pkg_name}::${pkg_name}-${pkg_link}
                ${pkg_find_name}::${pkg_find_name}-${pkg_link}
                ${pkg_name}-${pkg_link}
                ${pkg_find_name}-${pkg_link}
                )
    endif()
    list(REMOVE_DUPLICATES PKG_TARGET_HINTS)

    foreach (tgt ${PKG_DEPENDS})
        if(NOT TARGET ${tgt})
            list(APPEND PKG_MISSING_TARGET ${tgt})
        endif()
    endforeach()
    if(PKG_MISSING_TARGET)
        message(FATAL_ERROR "Could not install ${pkg_name}: dependencies missing [${PKG_MISSING_TARGET}]")
    endif()

    # Append to CMAKE_PREFIX_PATH so we can find the packages later
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${pkg_install_dir};${PKG_INSTALL_DIR}" CACHE INTERNAL "Paths for find_package lookup" FORCE)
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

    # Try finding config files before modules
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

    if(PKG_LIBRARY_NAMES)
        # This attempts to find <PackageName>_LIBRARY with given names before calling find_package
        # This is necessary for ZLIB
        find_library(${pkg_find_name}_LIBRARY
                NAMES ${PKG_LIBRARY_NAMES}
                HINTS ${pkg_install_dir} ${${pkg_find_name}_ROOT} ${${pkg_name}_ROOT}
                PATH_SUFFIXES ${pkg_name} ${pkg_find_name} lib ${pkg_find_name}/lib ${pkg_name}/lib
                NO_CMAKE_ENVIRONMENT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                )
        if(${pkg_find_name}_LIBRARY)
            message(DEBUG "Found ${pkg_find_name}_LIBRARY: ${${pkg_find_name}_LIBRARY}")
            find_package(${pkg_find_name} ${PKG_VERSION} ${COMPONENTS} ${PKG_COMPONENTS} ${QUIET})
        endif()
    elseif(PKG_MODULE)
        if(NOT ${pkg_find_name}_FOUND)
            find_package(${pkg_find_name} ${PKG_VERSION} ${COMPONENTS} ${PKG_COMPONENTS} ${QUIET})
        endif()
    else()
        if(NOT ${pkg_find_name}_FOUND)
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
    endif()
    # Set _FOUND variables for alternate names and components
    if(NOT ${pkg_name}_FOUND)
        if(${pkg_find_name}_FOUND)
            set(${pkg_name}_FOUND TRUE)
        else()
            # Some packages, such as SZIP, may only set each components as found but not the package itself,
            # with variables like SZIP_shared_FOUND and so on
            foreach(comp ${COMPONENTS})
                if(${pkg_name}_${comp}_FOUND)
                    set(${pkg_name}_FOUND TRUE)
                endif()
            endforeach()
        endif()
    endif()
    # Check if the package was found
    if(${pkg_name}_FOUND)
        if(PKG_DEPENDS)
            target_link_libraries(${pkg_target_name} INTERFACE ${PKG_DEPENDS})
        endif()
        if(PKG_CHECK)
            check_compile(${pkg_name} ${pkg_target_name} ${PROJECT_SOURCE_DIR}/cmake/compile/${pkg_name}.cpp)
            if(PKG_DEBUG AND NOT check_compile_${pkg_name} AND EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log")
                file(READ "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log" ERROR_LOG)
                message(STATUS "CMakeError.log: \n ${ERROR_LOG}")
            endif()
        endif()
        foreach(tgt ${PKG_TARGET_HINTS})
            if(TARGET ${tgt})
                set(PKG_${pkg_name}_TARGET ${tgt})
                set(PKG_${pkg_name}_TARGET ${tgt} PARENT_SCOPE)
                set(PKG_${pkg_name}_FOUND TRUE PARENT_SCOPE)
                set(PKG_${pkg_find_name}_TARGET ${tgt} PARENT_SCOPE)
                set(PKG_${pkg_find_name}_FOUND TRUE PARENT_SCOPE)
                set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS};${tgt})
                list(REMOVE_DUPLICATES PKG_INSTALLED_TARGETS)
                set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS} PARENT_SCOPE)
                mark_as_advanced(PKG_${pkg_name}_TARGET)
                mark_as_advanced(PKG_${pkg_find_name}_TARGET)
                mark_as_advanced(PKG_INSTALLED_TARGETS)
                break()
            endif()
        endforeach()
        if(TARGET ${PKG_${pkg_name}_TARGET})
            message(DEBUG "Found ${pkg_name}: [${PKG_${pkg_name}_TARGET}]")
        else()
            message(WARNING "Found ${pkg_name} but no target matches [${PKG_TARGET_HINTS}]")
        endif()
        return()
    endif()

    message(STATUS "${pkg_name} will be installed into ${pkg_install_dir}")

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
        message(STATUS "Got non-zero exit code while configuring ${pkg_name}")
        message(STATUS  "build_dir             : ${pkg_build_dir}")
        message(STATUS  "install_dir           : ${pkg_install_dir}")
        message(STATUS  "extra_flags           : ${extra_flags}")
        message(STATUS  "config_result         : ${config_result}")
        message(FATAL_ERROR "Failed to configure ${pkg_name}")
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
                message(STATUS "Got non-zero exit code while building package: ${pkg_name}")
                message(STATUS  "build config      : ${config}")
                message(STATUS  "build dir         : ${pkg_build_dir}")
                message(STATUS  "install dir       : ${pkg_install_dir}")
                message(STATUS  "cmake args        : ${PKG_CMAKE_ARGS}")
                message(STATUS  "build result      : ${build_result}")
                message(FATAL_ERROR "Failed to build package: ${pkg_name}")
            endif()
        endforeach()
    else()
        # This is for single-config generators such as Unix Makefiles and Ninja
        execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads}
                WORKING_DIRECTORY "${pkg_build_dir}"
                RESULT_VARIABLE build_result
                )
        if(build_result)
            message(STATUS "Got non-zero exit code while building package: ${pkg_name}")
            message(STATUS  "build dir         : ${pkg_build_dir}")
            message(STATUS  "install dir       : ${pkg_install_dir}")
            message(STATUS  "cmake args        : ${PKG_CMAKE_ARGS}")
            message(STATUS  "build result      : ${build_result}")
            message(FATAL_ERROR "Failed to build package: ${pkg_name}")
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

    if(PKG_DEPENDS)
        target_link_libraries(${pkg_target_name} INTERFACE ${PKG_DEPENDS})
    endif()
    if(PKG_CHECK)
        check_compile(${pkg_name} ${pkg_target_name} ${PROJECT_SOURCE_DIR}/cmake/compile/${pkg_name}.cpp)
        if(PKG_DEBUG AND NOT check_compile_${pkg_name} AND EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log")
            file(READ "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log" ERROR_LOG)
            message(STATUS "CMakeError.log: \n ${ERROR_LOG}")
        endif()
    endif()
    foreach(tgt ${PKG_TARGET_HINTS})
        if(TARGET ${tgt})
            set(PKG_${pkg_name}_TARGET ${tgt})
            set(PKG_${pkg_name}_TARGET ${tgt} PARENT_SCOPE)
            set(PKG_${pkg_name}_FOUND TRUE PARENT_SCOPE)
            set(PKG_${pkg_find_name}_TARGET ${tgt} PARENT_SCOPE)
            set(PKG_${pkg_find_name}_FOUND TRUE PARENT_SCOPE)
            set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS};${tgt})
            list(REMOVE_DUPLICATES PKG_INSTALLED_TARGETS)
            set(PKG_INSTALLED_TARGETS ${PKG_INSTALLED_TARGETS} PARENT_SCOPE)
            mark_as_advanced(PKG_${pkg_name}_TARGET)
            mark_as_advanced(PKG_${pkg_find_name}_TARGET)
            mark_as_advanced(PKG_INSTALLED_TARGETS)
            break()
        endif()
    endforeach()
    if(TARGET ${PKG_${pkg_name}_TARGET})
        message(DEBUG "Found ${pkg_name}: [${PKG_${pkg_name}_TARGET}]")
    else()
        message(WARNING "Found ${pkg_name} but no target matches [${PKG_TARGET_HINTS}]")
    endif()
endfunction()

