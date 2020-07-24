 
cmake_minimum_required(VERSION 3.12)
project(h5pp VERSION 1.8.0
        DESCRIPTION "A C++17 wrapper for HDF5"
        HOMEPAGE_URL "https://github.com/DavidAce/h5pp"
        LANGUAGES CXX
        )

option(BUILD_SHARED_LIBS               "Builds shared libraries"                                     OFF)
option(H5PP_BUILD_EXAMPLES             "Builds examples"                                             OFF)
option(H5PP_ENABLE_TESTS               "Enable testing"                                              OFF)
option(H5PP_IS_SUBPROJECT              "Use h5pp with add_subdirectory()"                            OFF)
option(H5PP_DEPS_IN_SUBDIR             "Install dependencies into CMAKE_INSTALL_PREFIX/<libname>"    OFF)
option(H5PP_PREFER_CONDA_LIBS          "Search for dependencies from anaconda first"                 OFF)
option(H5PP_PRINT_INFO                 "Print info during cmake configuration"                       OFF)
option(H5PP_ENABLE_EIGEN3              "Enables Eigen3 linear algebra library"                       OFF)
option(H5PP_ENABLE_SPDLOG              "Enables Spdlog for logging h5pp internal info to stdout"     OFF)
option(H5PP_ENABLE_MPI                 "Enables use of MPI (work in progress)"                       OFF)
option(H5PP_ENABLE_ASAN                "Enable runtime address sanitizer -fsanitize=address"         OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE "Use -fPIC when compiling shared libraries"                   ON)

# Make an "enum" for valid download methods: none find fetch find-or-fetch conan
set(H5PP_DOWNLOAD_METHODS_VALID none find fetch find-or-fetch conan)
set(H5PP_DOWNLOAD_METHOD none CACHE STRING "Download method for external dependencies")
set_property(CACHE H5PP_DOWNLOAD_METHOD PROPERTY STRINGS ${H5PP_DOWNLOAD_METHODS_VALID})
if (NOT H5PP_DOWNLOAD_METHOD IN_LIST H5PP_DOWNLOAD_METHODS_VALID)
    message(FATAL_ERROR "H5PP_DOWNLOAD_METHOD must be one of ${H5PP_DOWNLOAD_METHODS_VALID}")
endif ()


# Used when h5pp is included as subproject (e.g., as Git submodule/subtree) in the source
# tree of a project that uses it. Users may set the option H5PP_IS_SUBPROJECT
# before add_subdirectory(h5pp)
if (NOT H5PP_IS_SUBPROJECT)
    if ("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
        set(H5PP_IS_SUBPROJECT OFF)
    else ()
        message(STATUS "Detected h5pp as subproject")
        set(H5PP_IS_SUBPROJECT ON)
    endif ()
endif ()


# Set default install directory for h5pp and its dependencies
# Append directory <libname> to CMAKE_INSTALL_PREFIX
# Useful if you want to have separate directories for each libs
# and to easily delete them individually
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT H5PP_IS_SUBPROJECT)
    message(STATUS "Setting default install location: CMAKE_INSTALL_PREFIX --> ${CMAKE_BINARY_DIR}/install")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "..." FORCE)
endif ()

# Setup paths that find_package should search and
# let cmake find our Find<package>.cmake modules
include(cmake/SetupSearchPaths.cmake)



# Print cmake build info options and host properties
include(cmake/PrintBuildInfo.cmake)


# Define main target and auxiliary for partial consumption
add_library(h5pp INTERFACE)
add_library(headers INTERFACE)
add_library(deps INTERFACE)
add_library(flags INTERFACE)
list(APPEND H5PP_TARGETS h5pp headers deps flags)


########################## IMPORTANT #############################
### Preempt Threads::Threads                                   ###
### Threads::Threads is looked for in dependencies, so we make ###
### it right before it's done wrong, i.e. by linking with      ###
### "pthread" instead of "-lpthread". If this is undesirable   ###
### you can preempt it yourself similarly.                     ###
### The reason behind this is that downstream I often need to  ###
### avoid a static linking segfault by using:                  ###
###     -Wl, -whole-archive pthread -Wl, -no-whole-archive     ###
### and cmake is unable to identify "-lpthread" as being the   ###
### same as pthread --> multiple declaration errors. Note that ###
### CMake changes the link flags into                          ###
### -Wl, -whole-archive -lpthread -Wl, -no-whole-archive       ###
### anyway. If an entire dependency tree had only "-lpthread"  ###
### or only "pthread" this wouldn't be a problem, but          ###
### unfortunately different dependencies do it differently.    ###
### Since conan prefers "pthread", I'll do the same here.      ###
##################################################################
### Read more about pthread segfault
### https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
if (NOT TARGET Threads::Threads)
    set(CMAKE_THREAD_PREFER_PTHREAD FALSE)
    set(THREADS_PREFER_PTHREAD_FLAG FALSE)
    find_package(Threads)
    if (TARGET Threads::Threads AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_target_properties(Threads::Threads PROPERTIES INTERFACE_LINK_LIBRARIES pthread)
        list(APPEND H5PP_TARGETS Threads::Threads)
    endif ()
endif ()

if (H5PP_ENABLE_MPI AND NOT WIN32)
    find_package(MPI)
    if (TARGET MPI::MPI_CXX)
        target_link_libraries(flags INTERFACE MPI::MPI_CXX)
    endif ()
endif ()

# On some HPC clusters Clang needs path to gnu gcc toolchain because it's not in path
if (GCC_TOOLCHAIN AND ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    target_compile_options(flags INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
endif ()

# Add def for download method for use in installed h5ppConfig.cmake
target_compile_definitions(deps INTERFACE H5PP_DOWNLOAD_METHOD=${H5PP_DOWNLOAD_METHOD})


# Check  #include<optional> or #include<experimental/optional>
include(${PROJECT_SOURCE_DIR}/cmake/CheckCXXOptional.cmake)
CheckCXXOptional()

# Required compiler features
target_compile_features(flags INTERFACE cxx_std_17)
target_compile_options(flags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive->) # Need this for and/or logical operators on VS
target_compile_definitions(flags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>) # Otherwise std::min and std::max will not work as expected
target_include_directories(headers INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/h5pp/include>)
target_include_directories(headers SYSTEM INTERFACE $<INSTALL_INTERFACE:include>)


#######################################
# Settings for sanitizers           ###
#######################################
if(H5PP_ENABLE_ASAN)
    target_compile_options(flags INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_libraries(flags INTERFACE -fsanitize=address)
endif()


#Try to find or fetch all dependencies
include(cmake/SetupDependenciesFind.cmake)
include(cmake/SetupDependenciesFetch.cmake)
include(cmake/SetupDependenciesConan.cmake)



# Link all targets to one main h5pp target (the only one that users should need)
# However, it's nice to have them separately also if need be.
target_link_libraries(h5pp INTERFACE headers deps flags)



# Print summary of CMake configuration
if (H5PP_PRINT_INFO)
    include(cmake/PrintTargetInfo.cmake)
    include(cmake/getExpandedTarget.cmake)
    expand_target_all_targets(h5pp H5PP_TARGETS_EXPANDED)
    message(STATUS "| H5PP TARGET SUMMARY")
    message(STATUS "|--------------------")
    foreach(tgt ${H5PP_TARGETS_EXPANDED})
        print_target_info(${tgt} "| ")
    endforeach()
endif()



if(H5PP_IS_SUBPROJECT)
    add_library(h5pp::h5pp ALIAS h5pp)
    add_library(h5pp::headers ALIAS headers)
    add_library(h5pp::deps ALIAS deps)
    add_library(h5pp::flags ALIAS h5pp)
else()

    # Install library
    # Read about this share path here https://cmake.org/cmake/help/v3.12/command/find_package.html
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/h5pp/include/ DESTINATION include COMPONENT headers)
    install(TARGETS h5pp headers deps flags EXPORT h5ppTargets)

    #Export the targets to a script
    install(EXPORT
            h5ppTargets
            NAMESPACE h5pp::
            DESTINATION share/h5pp/cmake)


    #Create a ConfigVersion.cmake file
    include(CMakePackageConfigHelpers)


    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
            INSTALL_DESTINATION share/h5pp/cmake
            PATH_VARS H5PP_SEARCH_PATHS
    )

    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppConfig.deps-fetch.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps-fetch.cmake
            INSTALL_DESTINATION share/h5pp/cmake
            PATH_VARS H5PP_SEARCH_PATHS
    )

    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppConfig.deps-conan.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps-conan.cmake
            INSTALL_DESTINATION share/h5pp/cmake
    )

    write_basic_package_version_file(
            ${CMAKE_BINARY_DIR}/h5ppConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
    )


    ##Install the config, configversion and custom find modules
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps-fetch.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps-conan.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindEigen3.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Findspdlog.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindHDF5.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindFilesystem.cmake
            DESTINATION share/h5pp/cmake
            COMPONENT config
            )


    if (H5PP_DOWNLOAD_METHOD MATCHES "conan")
        # Install conan related files to define dependency targets the same way
        # we've already done here.
        set(CONAN_INSTALL_FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt
                ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.txt
                ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt
                )
        foreach (conan_file ${CONAN_INSTALL_FILES})
            if (EXISTS "${conan_file}")
                install(FILES ${conan_file}
                        DESTINATION share/h5pp/cmake
                        COMPONENT config)
            endif ()
        endforeach ()
    endif ()

    # Uninstall target
    if(NOT TARGET uninstall)
        configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppUninstall.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/h5ppUninstall.cmake
                IMMEDIATE @ONLY)

        add_custom_target(uninstall
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/h5ppUninstall.cmake)
    endif()


endif ()


# Simple testing with ctest
if (NOT H5PP_IS_SUBPROJECT AND H5PP_ENABLE_TESTS AND TARGET h5pp)
    enable_testing()
    add_subdirectory(tests)
endif ()


# Build examples
if (NOT H5PP_IS_SUBPROJECT AND H5PP_BUILD_EXAMPLES AND TARGET h5pp)
    add_subdirectory(examples)
endif ()


# Use CPACK to generate .deb install file
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL headers config)

# Define apt dependencies that work with this library
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libhdf5-dev (>=1.10), libeigen3-dev (>=3.3.4), libspdlog-dev (>=1.3)")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A C++ wrapper for HDF5")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/DavidAce/h5pp")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DavidAce <aceituno@kth.se>")
set(CPACK_DEBIAN_PACKAGE_NAME "h5pp")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_GENERATOR "DEB")
include(CPack)
