cmake_minimum_required(VERSION 3.26)

function(pkg_message lvl msg)
    message(${lvl} "PkgInstall [${pkg_name}]: ${msg} ${ARGN}")
endfunction()

# Appends a CMake command that recreates one cache variable when the file is loaded.
# For example, var=MY_PATTERN, value=\.[0-9]+, type=STRING, help=Example produces:
#   set([=[MY_PATTERN]=] [=[\.[0-9]+]=] CACHE STRING [=[Example]=] FORCE)
#
# CMake's [=[...]=] syntax quotes literal text: unlike "...", it does not interpret
# backslashes or expand ${...} when the generated command is read. The brackets
# and equal signs are delimiters; they are not part of the stored variable/value.
#
# Any number of equal signs is allowed, provided both delimiters use the same
# number. Start with [=[...]=]. If an argument itself contains ]=], use [==[...]==]
# instead, adding more equal signs until none of the arguments contains the closing
# delimiter. This prevents text in a value or help string from ending it too early.
function(pkg_cache_entry file var value type help)
    set(eq "=")
    # Match a literal closing delimiter (]=], ]==], etc.) in any argument.
    while("${var}${value}${help}" MATCHES "\\]${eq}\\]")
        string(APPEND eq "=")
    endwhile()
    file(APPEND "${file}" "set([${eq}[${var}]${eq}] [${eq}[${value}]${eq}] CACHE ${type} [${eq}[${help}]${eq}] FORCE)\n")
endfunction()


# Dumps cached variables to PKG_INIT_CACHE_FILE so that we can propagate
# the current build configuration to dependencies
function(generate_init_cache)
    # Initialize installation-directory defaults before collecting build settings.
    include(GNUInstallDirs)
    set(PKG_INPUTS "")
    set(PKG_INIT_CACHE_FILE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/init-cache.cmake)
    set(PKG_INIT_CACHE_FILE ${PKG_INIT_CACHE_FILE} PARENT_SCOPE)
    file(WRITE  ${PKG_INIT_CACHE_FILE} "# These variables will initialize the CMake cache for subprocesses.\n")
    get_cmake_property(vars CACHE_VARIABLES)
    list(SORT vars)
    foreach(var ${vars})
        if(var MATCHES "CMAKE_CACHE|CMAKE_HOME|CMAKE_EXTRA|CMAKE_PROJECT|MACRO|^PKG_")
            continue()
        endif()
        get_property(help CACHE "${var}" PROPERTY HELPSTRING)
        get_property(type CACHE "${var}" PROPERTY TYPE)
        get_property(value CACHE "${var}" PROPERTY VALUE)
        pkg_cache_entry("${PKG_INIT_CACHE_FILE}" "${var}" "${value}" "${type}" "${help}")
        # Search results and cache metadata appear during configuration. They do
        # not change the build settings and must not force another installation.
        if(NOT type MATCHES "^(INTERNAL|STATIC)$"
                AND NOT var MATCHES "^(CPACK_|CXX_FILESYSTEM_)"
                AND NOT help MATCHES "^(Path to a (file|library)\\.|The directory containing a CMake configuration file)")
            string(SHA256 input_hash "${var}=${value}")
            string(APPEND PKG_INPUTS "${input_hash}\n")
        endif()
    endforeach()
    # Apply package overrides last, without changing the outer project's cache.
    foreach(arg IN LISTS PKG_CMAKE_ARGS)
        if(NOT arg MATCHES "^-D([^:=]+)(:([^=]+))?=(.*)$")
            pkg_message(FATAL_ERROR "CMAKE_ARGS expects -Dname[:type]=value; got: ${arg}")
        endif()
        set(var "${CMAKE_MATCH_1}")
        set(type "${CMAKE_MATCH_3}")
        set(value "${CMAKE_MATCH_4}")
        if(NOT type)
            get_property(type CACHE "${var}" PROPERTY TYPE)
            if(NOT type)
                set(type STRING)
            endif()
        endif()
        pkg_cache_entry("${PKG_INIT_CACHE_FILE}" "${var}" "${value}" "${type}" "Package override")
        string(SHA256 input_hash "${var}=${value}")
        string(APPEND PKG_INPUTS "${input_hash}\n")
    endforeach()
    # Generator selection is stored as INTERNAL cache entries, but affects builds.
    string(APPEND PKG_INPUTS "${CMAKE_GENERATOR};${CMAKE_GENERATOR_PLATFORM};${CMAKE_GENERATOR_TOOLSET};${CMAKE_GENERATOR_INSTANCE}\n")
    string(APPEND PKG_INPUTS "${CMAKE_COMMAND};${CMAKE_VERSION}\n")
    set(PKG_INPUTS "${PKG_INPUTS}" PARENT_SCOPE)
endfunction()


# This function will configure, build and install a package at configure-time
# by running cmake in a subprocess. The current CMake configuration is transmitted
# by setting the flags manually.
function(pkg_install pkg_name)
    set(options "")
    set(oneValueArgs INSTALL_DIR BUILD_DIR)
    set(multiValueArgs CMAKE_ARGS)
    cmake_parse_arguments(PARSE_ARGV 1 PKG "${options}" "${oneValueArgs}" "${multiValueArgs}")
    if(PKG_UNPARSED_ARGUMENTS OR PKG_KEYWORDS_MISSING_VALUES)
        pkg_message(FATAL_ERROR "Unknown or incomplete arguments: ${PKG_UNPARSED_ARGUMENTS};${PKG_KEYWORDS_MISSING_VALUES}")
    endif()


    # Set directories
    if(NOT PKG_BUILD_DIR)
        set(PKG_BUILD_DIR ${CMAKE_BINARY_DIR}/pkg-build/${pkg_name})
    endif()

    # INSTALL_DIR overrides the shared dependency prefix for this package only.
    # Without a separate dependency prefix, follow the current installation prefix
    # rather than caching a copy that would become stale when it changes.
    if(NOT PKG_INSTALL_DIR)
        if(PKG_INSTALL_PREFIX)
            set(PKG_INSTALL_DIR "${PKG_INSTALL_PREFIX}")
        elseif(NOT CMAKE_INSTALL_PREFIX)
            set(PKG_INSTALL_DIR ${CMAKE_BINARY_DIR}/pkg-install)
        else()
            set(PKG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
        endif()
    endif()

    # Append to CMAKE_PREFIX_PATH so we can find the packages later
    pkg_message(DEBUG "Appending to CMAKE_PREFIX_PATH: ${PKG_INSTALL_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${PKG_INSTALL_DIR}")
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE INTERNAL "Paths for find_package lookup" FORCE)


    # All projects use the same lock to avoid building packages at the same time.
    # Check the installation after acquiring it: another process may just have finished.
    if(DEFINED ENV{HOME})
        pkg_message(STATUS "Waiting for lock: $ENV{HOME}/cmake.pkg-install.lock")
        file(LOCK "$ENV{HOME}/cmake.pkg-install.lock" GUARD FUNCTION TIMEOUT 600)
    elseif(DEFINED ENV{USERPROFILE})
        pkg_message(STATUS "Waiting for lock: $ENV{USERPROFILE}/cmake.pkg-install.lock")
        file(LOCK "$ENV{USERPROFILE}/cmake.pkg-install.lock" GUARD FUNCTION TIMEOUT 600)
    else()
        pkg_message(FATAL_ERROR "Cannot determine a home directory for the package-build lock")
    endif()

    pkg_message(STATUS "Checking existing installation: ${PKG_INSTALL_DIR}")

    # Generate an init cache to propagate the current configuration
    generate_init_cache()

    # The success file records the build inputs and the files that were installed.
    # Include all package recipes so changes to zlib/libaec also rebuild HDF5.
    file(GLOB_RECURSE PKG_RECIPES LIST_DIRECTORIES FALSE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external_*/*")
    list(APPEND PKG_RECIPES "${CMAKE_CURRENT_FUNCTION_LIST_FILE}" "${CMAKE_TOOLCHAIN_FILE}")
    list(SORT PKG_RECIPES)
    foreach(recipe IN LISTS PKG_RECIPES)
        if(EXISTS "${recipe}")
            file(SHA256 "${recipe}" recipe_hash)
            string(APPEND PKG_INPUTS "${recipe}:${recipe_hash}\n")
        endif()
    endforeach()
    string(SHA256 PKG_INPUT_HASH "${PKG_INPUTS};${PKG_INSTALL_DIR};${CMAKE_C_COMPILER_VERSION};${CMAKE_CXX_COMPILER_VERSION}")
    # Changing the filename makes ExternalProject rerun its configure step too.
    set(PKG_CACHE_FILE "${PKG_BUILD_DIR}/init-${PKG_INPUT_HASH}.cmake")
    file(MAKE_DIRECTORY "${PKG_BUILD_DIR}")
    file(RENAME "${PKG_INIT_CACHE_FILE}" "${PKG_CACHE_FILE}")
    set(PKG_INIT_CACHE_FILE "${PKG_CACHE_FILE}")
    set(PKG_SUCCESS_FILE "${PKG_INSTALL_DIR}/.pkg_install_success_${pkg_name}")
    if(EXISTS "${PKG_SUCCESS_FILE}")
        file(STRINGS "${PKG_SUCCESS_FILE}" INSTALLED_FILES)
        if(INSTALLED_FILES)
            list(POP_FRONT INSTALLED_FILES installed_hash)
            set(installed_files_exist TRUE)
            foreach(installed_file IN LISTS INSTALLED_FILES)
                if(NOT EXISTS "${installed_file}")
                    set(installed_files_exist FALSE)
                    break()
                endif()
            endforeach()
            if(installed_hash STREQUAL PKG_INPUT_HASH AND installed_files_exist)
                pkg_message(VERBOSE "Already installed: ${pkg_name} (found manifest: ${PKG_SUCCESS_FILE})")
                return()
            endif()
        endif()
    endif()
    # A failed rebuild must not leave an earlier success record behind. Removing
    # install stamps also restores missing files when ExternalProject is up to date.
    file(REMOVE "${PKG_SUCCESS_FILE}")
    file(GLOB_RECURSE install_stamps "${PKG_BUILD_DIR}/src/*-install")
    if(install_stamps)
        file(REMOVE ${install_stamps})
    endif()
    set(INSTALLED_FILES)

    pkg_message(STATUS "${pkg_name} will be installed into ${PKG_INSTALL_DIR}")

    # Configure the package
    pkg_message(STATUS "Configuring in ${PKG_BUILD_DIR}")
    execute_process(COMMAND  ${CMAKE_COMMAND} -E make_directory ${PKG_BUILD_DIR})
    execute_process(COMMAND  ${CMAKE_COMMAND} -E remove ${PKG_BUILD_DIR}/CMakeCache.txt)
    execute_process(
            COMMAND
            ${CMAKE_COMMAND}
            -C "${PKG_INIT_CACHE_FILE}"                # For the subproject in external_<libname>
            "-DINIT_CACHE_FILE=${PKG_INIT_CACHE_FILE}" # For externalproject_add inside the subproject
            "-DCMAKE_INSTALL_PREFIX:PATH=${PKG_INSTALL_DIR}"
            -DCMAKE_POLICY_DEFAULT_CMP0074:STRING=NEW # "Honor <PackageName>_ROOT"
            -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW # "Use MSVC_RUNTIME_LIBRARY", fixes spdlog on MSVC
            -DCMAKE_POLICY_DEFAULT_CMP0135:STRING=NEW # "Use DOWNLOAD_EXTRACT_TIMESTAMP"
            -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=TRUE # "Otherwise fails with -G Ninja"
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/external_${pkg_name}
            WORKING_DIRECTORY ${PKG_BUILD_DIR}
            RESULT_VARIABLE config_result
    )
    if(config_result)
        pkg_message(STATUS "Got non-zero exit code while configuring ${pkg_name}")
        pkg_message(STATUS "build_dir             : ${PKG_BUILD_DIR}")
        pkg_message(STATUS "install_dir           : ${PKG_INSTALL_DIR}")
        pkg_message(STATUS "cmake args            : ${PKG_CMAKE_ARGS}")
        pkg_message(STATUS "config_result         : ${config_result}")
        pkg_message(FATAL_ERROR "Failed to configure ${pkg_name}")
    endif()

    # Make sure to do multithreaded builds if possible
    if(DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL} AND NOT "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}" STREQUAL "")
        set(num_threads $ENV{CMAKE_BUILD_PARALLEL_LEVEL})
    elseif(CMAKE_BUILD_PARALLEL_LEVEL)
        set(num_threads ${CMAKE_BUILD_PARALLEL_LEVEL})
    else()
        cmake_host_system_information(RESULT num_threads QUERY NUMBER_OF_PHYSICAL_CORES)
    endif()

    # Build the package
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(multi_config)
        # This branch is for multi-config generators such as Visual Studio 16 2019
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            pkg_message(STATUS "Building and installing ${config} in ${PKG_BUILD_DIR}")
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
            # Install manifests are overwritten by the next configuration.
            file(GLOB_RECURSE INSTALL_MANIFEST "${PKG_BUILD_DIR}/install_manifest*.txt")
            foreach(manifest ${INSTALL_MANIFEST})
                get_filename_component(manifest_filename ${manifest} NAME_WE)
                pkg_message(STATUS "Copying install manifest: ${manifest}")
                configure_file("${manifest}" "${CMAKE_BINARY_DIR}/${manifest_filename}_${pkg_name}_${config}.txt" COPYONLY)
                file(STRINGS "${manifest}" installed_files)
                list(APPEND INSTALLED_FILES ${installed_files})
            endforeach()
        endforeach()
    else()
        # This is for single-config generators such as Unix Makefiles and Ninja
        pkg_message(STATUS "Building and installing in ${PKG_BUILD_DIR}")
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
        # Copy the install manifest if it exists.
        file(GLOB_RECURSE INSTALL_MANIFEST "${PKG_BUILD_DIR}/install_manifest*.txt")
        foreach(manifest ${INSTALL_MANIFEST})
            get_filename_component(manifest_filename ${manifest} NAME_WE)
            pkg_message(STATUS "Copying install manifest: ${manifest}")
            configure_file("${manifest}" "${CMAKE_BINARY_DIR}/${manifest_filename}_${pkg_name}.txt" COPYONLY)
            file(STRINGS "${manifest}" installed_files)
            list(APPEND INSTALLED_FILES ${installed_files})
        endforeach()
    endif()

    if(NOT INSTALLED_FILES)
        pkg_message(FATAL_ERROR "No installed files were recorded; not marking this package complete")
    endif()
    list(REMOVE_DUPLICATES INSTALLED_FILES)
    list(JOIN INSTALLED_FILES "\n" INSTALLED_FILES)
    file(WRITE "${PKG_SUCCESS_FILE}" "${PKG_INPUT_HASH}\n${INSTALLED_FILES}\n")
endfunction()
