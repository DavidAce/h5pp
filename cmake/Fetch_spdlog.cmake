
find_package(spdlog 1.3.1)


if(TARGET spdlog::spdlog)
#    return()
elseif(TARGET spdlog)
    add_library(spdlog::spdlog ALIAS spdlog)
#    return()
elseif (DOWNLOAD_METHOD MATCHES "native")
    message(STATUS "Spdlog will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
    build_dependency(spdlog  "${spdlog_install_prefix}" "" "")
    find_package(spdlog 1.3
            HINTS ${spdlog_install_prefix}
            PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR}/cmake/spdlog spdlog spdlog/${CMAKE_INSTALL_LIBDIR} spdlog/share spdlog/cmake
            NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY )

    if(TARGET spdlog::spdlog)
        message(STATUS "spdlog installed successfully")
    else()
        message(FATAL_ERROR "Spdlog could not be downloaded.")
    endif()

else()
    message(STATUS "Dependency spdlog not found in your system. Set DOWNLOAD_METHOD to one of 'conan|native'")

endif()