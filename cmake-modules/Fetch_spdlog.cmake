
include(GNUInstallDirs)
message(STATUS "Fetch spdlog given directory spdlog_DIR: ${spdlog_DIR}")
find_package(spdlog 1.3
        PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/spdlog/${CMAKE_INSTALL_LIBDIR}/cmake/spdlog ${spdlog_DIR} ${DIRECTORY_HINTS}
        PATH_SUFFIXES spdlog spdlog/${CMAKE_INSTALL_LIBDIR} spdlog/share spdlog/cmake
        NO_DEFAULT_PATH  )

if(spdlog_FOUND AND TARGET spdlog::spdlog)
    message(STATUS "spdlog found in system")
#    include(cmake-modules/PrintTargetProperties.cmake)
#    print_target_properties(spdlog::spdlog)

elseif (DOWNLOAD_MISSING)
    message(STATUS "Spdlog will be installed into ${H5PP_INSTALL_DIR_THIRD_PARTY}/spdlog on first build.")
    include(cmake-modules/BuildThirdParty.cmake)
    build_third_party(
            "spdlog"
            "${H5PP_CONFIG_DIR_THIRD_PARTY}"
            "${H5PP_BUILD_DIR_THIRD_PARTY}"
            "${H5PP_INSTALL_DIR_THIRD_PARTY}"
            ""
    )
    find_package(spdlog 1.3 NO_DEFAULT_PATH PATHS ${H5PP_INSTALL_DIR_THIRD_PARTY}/spdlog/${CMAKE_INSTALL_LIBDIR}/cmake/spdlog)
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