
find_package(ghcFilesystem HINTS ${CMAKE_INSTALL_PREFIX} PATH_SUFFIXES ghcFilesystem)
if  (NOT TARGET ghcFilesystem::ghc_filesystem)
    message(STATUS "ghcFilesystem will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
    build_dependency(ghcFilesystem  "${CMAKE_INSTALL_PREFIX}" "" "")
    find_package(ghcFilesystem HINTS ${CMAKE_INSTALL_PREFIX}
                PATH_SUFFIXES ghcFilesystem
                NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY )

    if(TARGET ghcFilesystem::ghc_filesystem)
        message(STATUS "ghcFilesystem installed successfully")
    else()
        message(FATAL_ERROR "ghcFilesystem could not be downloaded.")
    endif()
endif()