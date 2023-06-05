include("${CMAKE_CURRENT_LIST_DIR}/cmake_installer.cmake")

cmake_language(
        SET_DEPENDENCY_PROVIDER pkg_install_dependencies
        SUPPORTED_METHODS FIND_PACKAGE
)