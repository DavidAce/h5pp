 
cmake_minimum_required(VERSION 3.15)
project(h5pp VERSION 1.11.1
        DESCRIPTION "A C++17 wrapper for HDF5"
        HOMEPAGE_URL "https://github.com/DavidAce/h5pp"
        LANGUAGES C CXX
        )

include(cmake/DeprecationWarnings.cmake) # Warn if user passes deprecated settings

# Used when h5pp is included as subproject (e.g., as Git submodule/subtree) in the source
# tree of a project that uses it. Users may set the option H5PP_IS_SUBPROJECT
# before add_subdirectory(h5pp)
if (NOT DEFINED H5PP_IS_SUBPROJECT)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        option(H5PP_IS_SUBPROJECT "Use h5pp with add_subdirectory()" OFF)
    else ()
        message(STATUS "Detected h5pp as subproject")
        option(H5PP_IS_SUBPROJECT "Use h5pp with add_subdirectory()" ON)
    endif ()
endif ()

option(BUILD_SHARED_LIBS                "Builds shared libraries"                                                 OFF)
option(H5PP_ENABLE_EIGEN3               "Enables Eigen3 linear algebra library"                                   OFF)
option(H5PP_ENABLE_FMT                  "Enables the {fmt} formatting library"                                    OFF)
option(H5PP_ENABLE_SPDLOG               "Enables Spdlog for logging h5pp internal info to stdout (implies fmt)"   OFF)
option(H5PP_ENABLE_MPI                  "Enables use of MPI (work in progress)"                                   OFF)
option(H5PP_ENABLE_ASAN                 "Enable runtime address sanitizer -fsanitize=address"                     OFF)
option(H5PP_ENABLE_PCH                  "Enable precompiled headers (if supported) to speed up test compilation"  OFF)
option(H5PP_ENABLE_CCACHE               "Enable ccache (if available) to speed up test compilation"               OFF)
option(H5PP_ENABLE_COVERAGE             "Enable test coverage"                                                    OFF)
option(H5PP_USE_FLOAT128                "Add support for __float128 if the compiler supports it"                  OFF)
option(H5PP_USE_QUADMATH                "Link the quadmath library for more advanced use of __float128"           OFF)
option(H5PP_BUILD_EXAMPLES              "Builds examples"                                                         OFF)
option(H5PP_BUILD_DOCS                  "Builds documentation (Requires doxygen, sphinx and breathe)"             OFF)
option(H5PP_ENABLE_TESTS                "Enable testing"                                                          OFF)
option(H5PP_IS_SUBPROJECT               "Use h5pp with add_subdirectory()"                                        OFF)
option(H5PP_PREFIX_ADD_PKGNAME          "Install h5pp and dependencies into <CMAKE_INSTALL_PREFIX>/<PackageName>" OFF)
option(CMAKE_POSITION_INDEPENDENT_CODE  "Use -fPIC when compiling shared libraries"                               ON)

if(H5PP_ENABLE_SPDLOG)
    set(H5PP_ENABLE_FMT ON CACHE INTERNAL "H5PP_ENABLE_SPDLOG:ON implies H5PP_ENABLE_FMT:ON)" FORCE)
endif()


# Setup paths that find_package should search, or where to find the conan executable
include(cmake/SetupPaths.cmake)

# Print cmake build info options and host properties
include(cmake/PrintBuildInfo.cmake)

# Check  #include<optional> or #include<experimental/optional>
include(cmake/CheckCXXOptional.cmake)
CheckCXXOptional()


# Define main target and auxiliary targets for modular use
add_library(h5pp INTERFACE)
add_library(headers INTERFACE)
add_library(deps INTERFACE)
add_library(flags INTERFACE)

# Required compiler features
target_compile_features(flags INTERFACE cxx_std_17)
target_compile_options(flags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive->) # Need this for and/or logical operators on VS
target_compile_options(flags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)        # Need this for try/catch without warnings on VS
target_compile_definitions(flags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>) # Otherwise std::min and std::max will not work as expected
target_include_directories(headers INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(headers SYSTEM INTERFACE $<INSTALL_INTERFACE:include>)


# Setup sanitizer
if(H5PP_ENABLE_ASAN)
    target_compile_options(flags INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_libraries(flags INTERFACE -fsanitize=address)
endif()


# Find or install all dependencies
include(cmake/SetupDependencies.cmake)

# Link all targets to one main h5pp target (the only one that users should need)
# However, it's nice to have them separately also if need be.
target_link_libraries(h5pp INTERFACE headers deps flags)


# Print summary of CMake configuration
include(cmake/PrintTargetInfo.cmake)
print_target_summary(h5pp)


if(H5PP_IS_SUBPROJECT)
    add_library(h5pp::h5pp ALIAS h5pp)
    add_library(h5pp::headers ALIAS headers)
    add_library(h5pp::deps ALIAS deps)
    add_library(h5pp::flags ALIAS h5pp)
else()

    # Install library
    # Read about this share path here https://cmake.org/cmake/help/v3.12/command/find_package.html
    include(GNUInstallDirs)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT h5pp-include)
    install(TARGETS h5pp headers deps flags EXPORT h5ppTargets)

    #Export the targets to a script
    install(EXPORT
            h5ppTargets
            NAMESPACE h5pp::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/h5pp)

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/h5pp
    )
    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppDeps.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppDeps.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/h5pp
    )

    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppVars.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppVars.cmake
            @ONLY)

    write_basic_package_version_file(
            ${CMAKE_BINARY_DIR}/h5ppConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
    )

    #Install the config, configversion and custom find modules
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppDeps.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppVars.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HDF5TargetUtils.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/FindEigen3.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/FindFilesystem.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/Findfmt.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/Findspdlog.cmake
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/FindHDF5.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/h5pp
            COMPONENT h5pp-config
            )


    # Uninstall target
    if(NOT TARGET h5pp-uninstall)
        configure_file(
                ${CMAKE_CURRENT_SOURCE_DIR}/cmake/h5ppUninstall.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/h5ppUninstall.cmake
                IMMEDIATE @ONLY)

        add_custom_target(h5pp-uninstall
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/h5ppUninstall.cmake)
    endif()


endif ()


# Simple testing with ctest
if (H5PP_ENABLE_TESTS AND TARGET h5pp)
    enable_testing()
    add_subdirectory(tests)
endif ()


# Build examples
if (H5PP_BUILD_EXAMPLES AND TARGET h5pp)
    add_subdirectory(examples)
endif ()


# Build docs
if(H5PP_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Use CPACK to generate .deb install file
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL h5pp-include h5pp-config)

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


