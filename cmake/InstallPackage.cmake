cmake_minimum_required(VERSION 3.18)

# Dumps cached variables to H5PP_INIT_CACHE_FILE so that we can propagate
# the current build configuration to dependencies
function(generate_init_cache)
    set(H5PP_INIT_CACHE_FILE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/init-cache.cmake)
    set(H5PP_INIT_CACHE_FILE ${H5PP_INIT_CACHE_FILE} PARENT_SCOPE)
    file(WRITE  ${H5PP_INIT_CACHE_FILE} "# These variables will initialize the CMake cache for subprocesses.\n")
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
        file(APPEND ${H5PP_INIT_CACHE_FILE} "set(${var} \"${${var}}\" CACHE ${type} \"${help}\" FORCE)\n")
    endforeach()
endfunction()


# This function will configure, build and install a package at configure-time
# by running cmake in a subprocess. The current CMake configuration is transmitted
# by setting the flags manually.
function(install_package package_name install_dir extra_flags)
    set(build_dir ${H5PP_DEPS_BUILD_DIR}/${package_name})
    if (H5PP_PREFIX_ADD_PKGNAME)
        set(install_dir ${install_dir}/${package_name})
    endif()

    set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
    set(CMAKE_CXX_STANDARD_REQUIRED TRUE CACHE BOOL "")
    set(CMAKE_CXX_EXTENSIONS FALSE CACHE BOOL "")

    # Set policies for CMakeLists in packages that require older CMake versions
    set(CMAKE_POLICY_DEFAULT_CMP0074 NEW CACHE STRING "Honor <PackageName>_ROOT")
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW CACHE STRING "Use MSVC_RUNTIME_LIBRARY") # Fixes spdlog on MSVC

    # Generate an init cache to propagate the current configuration
    generate_init_cache()

    # Configure the package
    execute_process( COMMAND  ${CMAKE_COMMAND} -E make_directory ${build_dir})
    execute_process( COMMAND  ${CMAKE_COMMAND} -E remove ${build_dir}/CMakeCache.txt)
    execute_process(
            COMMAND
            ${CMAKE_COMMAND}
            -C ${H5PP_INIT_CACHE_FILE}                # For the subproject in external_<libname>
            -DINIT_CACHE_FILE=${H5PP_INIT_CACHE_FILE} # For externalproject_add inside the subproject
            -DCMAKE_INSTALL_PREFIX:PATH=${install_dir}
            ${extra_flags}
            ${PROJECT_SOURCE_DIR}/cmake/external_${package_name}
            WORKING_DIRECTORY ${build_dir}
            RESULT_VARIABLE config_result
    )
    if(config_result)
        message(STATUS "Got non-zero exit code while configuring ${package_name}")
        message(STATUS  "build_dir         : ${build_dir}")
        message(STATUS  "install_dir       : ${install_dir}")
        message(STATUS  "extra_flags       : ${extra_flags}")
        message(STATUS  "config_result     : ${config_result}")
        message(FATAL_ERROR "Failed to configure ${package_name}")
    endif()


    # Make sure to do multithreaded builds if possible
    include(cmake/GetNumThreads.cmake)
    get_num_threads(num_threads)

    # Build the package
    if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        # This branch is for multi-config generators such as Visual Studio 16 2019
        foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
            execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads} --config ${CONFIG}
                    WORKING_DIRECTORY "${build_dir}"
                    RESULT_VARIABLE build_result
            )
            if(build_result)
                message(STATUS "Got non-zero exit code while building package: ${package_name}")
                message(STATUS  "build_type        : ${CONFIG}")
                message(STATUS  "build_dir         : ${build_dir}")
                message(STATUS  "install_dir       : ${install_dir}")
                message(STATUS  "extra_flags       : ${extra_flags}")
                message(STATUS  "build_result      : ${build_result}")
                message(FATAL_ERROR "Failed to build package: ${package_name}")
            endif()
        endforeach()
    else()
        # This is for single-config generators such as Unix Makefiles and Ninja
        execute_process(COMMAND  ${CMAKE_COMMAND} --build . --parallel ${num_threads}
                WORKING_DIRECTORY "${build_dir}"
                RESULT_VARIABLE build_result
                )
        if(build_result)
            message(STATUS "Got non-zero exit code while building package: ${package_name}")
            message(STATUS  "build_dir         : ${build_dir}")
            message(STATUS  "install_dir       : ${install_dir}")
            message(STATUS  "extra_flags       : ${extra_flags}")
            message(STATUS  "build_result      : ${build_result}")
            message(FATAL_ERROR "Failed to build package: ${package_name}")
        endif()
    endif()


    # Copy the install manifest if it exists
    file(GLOB_RECURSE INSTALL_MANIFEST "${build_dir}/*/install_manifest.txt")
    if(INSTALL_MANIFEST)
        message(STATUS "Copying install manifest: ${INSTALL_MANIFEST}")
        configure_file(${INSTALL_MANIFEST} ${CMAKE_CURRENT_BINARY_DIR}/install_manifest_${package_name}.txt)
    endif()
    message(STATUS "Finished building h5pp dependency: ${package_name}")
endfunction()