
include(GNUInstallDirs)
message(STATUS "Fetch spdlog given directory spdlog_DIR: ${spdlog_DIR}")
find_package(spdlog 1.3
        PATHS ${DIRECTORY_HINTS} ${spdlog_DIR}
        PATH_SUFFIXES ${spdlog_suffix}${CMAKE_INSTALL_LIBDIR}/cmake/spdlog spdlog spdlog/${CMAKE_INSTALL_LIBDIR} spdlog/share spdlog/cmake
        NO_DEFAULT_PATH  )

if(spdlog_FOUND AND TARGET spdlog::spdlog)
    message(STATUS "spdlog found in system")
#    include(cmake-modules/PrintTargetProperties.cmake)
#    print_target_properties(spdlog::spdlog)

elseif (DOWNLOAD_MISSING)
    message(STATUS "Spdlog will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake-modules/BuildDependency.cmake)
    build_dependency(spdlog "")
    find_package(spdlog 1.3
            PATHS ${DIRECTORY_HINTS} ${spdlog_DIR}
            PATH_SUFFIXES ${spdlog_suffix}${CMAKE_INSTALL_LIBDIR}/cmake/spdlog spdlog spdlog/${CMAKE_INSTALL_LIBDIR} spdlog/share spdlog/cmake
            NO_DEFAULT_PATH  )

    if(spdlog_FOUND AND TARGET spdlog::spdlog)
        message(STATUS "spdlog installed successfully")
#        include(cmake-modules/PrintTargetProperties.cmake)
#        print_target_properties(spdlog::spdlog)
    else()
        message(STATUS "config_result: ${config_result}")
        message(STATUS "build_result: ${build_result}")
        message(FATAL_ERROR "Spdlog could not be downloaded.")
    endif()

else()
    message(STATUS "Dependency spdlog not found and DOWNLOAD_MISSING is OFF")

endif()