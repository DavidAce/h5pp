
include(GNUInstallDirs)
#find_package(spdlog 1.3
#        HINTS  ${spdlog_DIR} ${CONDA_HINTS}
#        PATHS  ${spdlog_DIR} ${DIRECTORY_HINTS}
#        PATH_SUFFIXES ${spdlog_suffix}${CMAKE_INSTALL_LIBDIR}/cmake/spdlog spdlog spdlog/${CMAKE_INSTALL_LIBDIR} spdlog/share spdlog/cmake)

if(NOT TARGET spdlog::spdlog)
    find_path(SPDLOG_INCLUDE_DIR
            NAMES spdlog/spdlog.h
            HINTS ${spdlog_DIR} ${CONDA_HINTS}
            PATHS /usr /usr/local ${spdlog_DIR} ${DIRECTORY_HINTS}
            PATH_SUFFIXES include spdlog/include
            )
    # Check for a file in new enough spdlog versions
    find_path(SPDLOG_COLOR_SINKS
            NAMES spdlog/sinks/stdout_color_sinks.h
            HINTS ${spdlog_DIR} ${CONDA_HINTS}
            PATHS /usr /usr/local ${spdlog_DIR} ${DIRECTORY_HINTS} ${SPDLOG_INCLUDE_DIR}
            PATH_SUFFIXES include spdlog/include
            )
    if(SPDLOG_INCLUDE_DIR AND SPDLOG_COLOR_SINKS)
        set(spdlog_FOUND TRUE)
        add_library(spdlog::spdlog INTERFACE IMPORTED)
        target_include_directories(spdlog::spdlog INTERFACE ${SPDLOG_INCLUDE_DIR})
    endif()
endif()



if(TARGET spdlog::spdlog)
    message(STATUS "spdlog found")
#    include(cmake-modules/PrintTargetProperties.cmake)
#    print_target_properties(spdlog::spdlog)

elseif (DOWNLOAD_MISSING)
    message(STATUS "Spdlog will be installed into ${CMAKE_INSTALL_PREFIX}")
    include(${PROJECT_SOURCE_DIR}/cmake-modules/BuildDependency.cmake)
    build_dependency(spdlog "")
    find_package(spdlog 1.3
            HINTS ${CMAKE_BINARY_DIR}/h5pp-deps-install
            PATH_SUFFIXES ${spdlog_suffix}${CMAKE_INSTALL_LIBDIR}/cmake/spdlog spdlog spdlog/${CMAKE_INSTALL_LIBDIR} spdlog/share spdlog/cmake
            NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY )

    if(TARGET spdlog::spdlog)
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