function(install_ghc_filesystem)
    find_package(
            ghc_filesystem
            HINTS ${H5PP_DEPS_INSTALL_DIR}
            PATH_SUFFIXES ghcFilesystem
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_PACKAGE_REGISTRY
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)

    if  (NOT ghc_filesystem_FOUND OR NOT TARGET ghcFilesystem::ghc_filesystem)
        message(STATUS "ghc_filesystem will be installed into ${H5PP_DEPS_INSTALL_DIR}")
        include(cmake/InstallPackage.cmake)
        install_package(ghc_filesystem  "${H5PP_DEPS_INSTALL_DIR}" "" "")
        find_package(ghc_filesystem HINTS
                    ${H5PP_DEPS_INSTALL_DIR}
                    PATH_SUFFIXES ghc_filesystem
                    NO_DEFAULT_PATH
                    REQUIRED)

        message(STATUS "ghcFilesystem installed successfully")
    endif()
endfunction()