find_package(
        ghcFilesystem
        HINTS ${H5PP_DEPS_INSTALL_DIR}
        PATH_SUFFIXES ghcFilesystem
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_PACKAGE_REGISTRY
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)

if  (NOT TARGET ghcFilesystem::ghc_filesystem)
    message(STATUS "ghcFilesystem will be installed into ${H5PP_DEPS_INSTALL_DIR}")
    include(cmake/InstallPackage.cmake)
    install_package(ghcFilesystem  "${H5PP_DEPS_INSTALL_DIR}" "" "")
    find_package(ghcFilesystem HINTS ${H5PP_DEPS_INSTALL_DIR}
                PATH_SUFFIXES ghcFilesystem
                NO_DEFAULT_PATH)

    if(TARGET ghcFilesystem::ghc_filesystem)
        message(STATUS "ghcFilesystem installed successfully")
    else()
        message(FATAL_ERROR "ghcFilesystem could not be downloaded.")
    endif()
endif()