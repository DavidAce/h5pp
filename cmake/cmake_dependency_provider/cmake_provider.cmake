# This provider builds HDF5 2.1.1, which requires CMake 3.26.
if(CMAKE_VERSION VERSION_LESS 3.26)
    message(FATAL_ERROR "The bundled CMake provider requires CMake >= 3.26 to build HDF5 2.1.1. Use preinstalled dependencies or the Conan provider with CMake 3.24.")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake_installer.cmake")

cmake_language(
        SET_DEPENDENCY_PROVIDER pkg_install_dependencies
        SUPPORTED_METHODS FIND_PACKAGE
)