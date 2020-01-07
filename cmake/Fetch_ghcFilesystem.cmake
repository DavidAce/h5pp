
find_package(ghcFilesystem HINTS ${ghcfilesystem_install_prefix} )


if(TARGET ghcFilesystem::ghc_filesystem)
    #    All done
else ()
    message(STATUS "ghcFilesystem will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
    build_dependency(ghcFilesystem  "${ghcfilesystem_install_prefix}" "" "")
    find_package(ghcFilesystem HINTS ${ghcfilesystem_install_prefix}
                NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY )

    if(TARGET ghcFilesystem::ghc_filesystem)
        message(STATUS "ghcFilesystem installed successfully")
    else()
        message(FATAL_ERROR "ghcFilesystem could not be downloaded.")
    endif()
endif()