# This provider builds HDF5 2.1.1, which requires CMake 3.26.
if(CMAKE_VERSION VERSION_LESS 3.26)
    message(FATAL_ERROR "h5pp requires CMake >= 3.26.")
endif()

# This file is loaded before project() initializes the usual installation prefix.
# Keep the default inside the build tree; preserve a prefix supplied by the user
# or toolchain, including CMake's environment default on versions that support it.
if(NOT DEFINED CMAKE_INSTALL_PREFIX AND NOT DEFINED ENV{CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/pkg-install" CACHE PATH "Installation directory")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake_installer.cmake")

cmake_language(
        SET_DEPENDENCY_PROVIDER pkg_install_dependencies
        SUPPORTED_METHODS FIND_PACKAGE
)