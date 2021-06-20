cmake_minimum_required(VERSION 3.15)

if(H5PP_PACKAGE_MANAGER MATCHES "cmake")
    include(cmake/InstallPackage.cmake)

    # Download fmt
    if (H5PP_ENABLE_FMT AND NOT TARGET fmt::fmt)
        # fmt is a dependency of spdlog
        # We fetch it here to get the latest version and to make sure we use the
        # compile library and avoid compile-time overhead in projects consuming h5pp.
        # Note that spdlog may already have been found in if H5PP_PACKAGE_MANAGER=find|cmake
        # then we can assume that spdlog already knows how and where to get fmt.
        set(fmt_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for fmt installed by h5pp")
        find_package(fmt 7.1.3 CONFIG
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(NOT fmt_FOUND OR NOT TARGET fmt::fmt)
            message(STATUS "fmt will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            install_package(fmt  "${H5PP_DEPS_INSTALL_DIR}" "${FMT_CMAKE_OPTIONS}")
            find_package(fmt 7.1.3 CONFIG REQUIRED
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_PACKAGE_REGISTRY
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        endif()
        if(TARGET fmt::fmt AND fmt_FOUND)
            list(APPEND H5PP_TARGETS fmt::fmt)
        else()
            message(FATAL_ERROR "fmt could not be downloaded and built from source")
        endif()
    endif()


    # Download spdlog
    if (H5PP_ENABLE_SPDLOG AND NOT TARGET spdlog::spdlog)
        set(spdlog_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for spdlog installed by h5pp")
        find_package(spdlog 1.8.5 CONFIG
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(NOT spdlog_FOUND OR NOT TARGET spdlog::spdlog)
            message(STATUS "Spdlog will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            if(fmt_FOUND AND TARGET fmt::fmt)
                get_target_property(FMT_INCLUDE_DIR  fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
                get_filename_component(fmt_ROOT ${FMT_INCLUDE_DIR}/.. ABSOLUTE)
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-DSPDLOG_FMT_EXTERNAL:BOOL=ON")
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-Dfmt_ROOT:PATH=${fmt_ROOT}")
            endif()
            install_package(spdlog  "${H5PP_DEPS_INSTALL_DIR}" "${SPDLOG_CMAKE_OPTIONS}")
            find_package(spdlog 1.8.5 CONFIG REQUIRED
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_PACKAGE_REGISTRY
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
            message(STATUS "spdlog installed successfully")
        endif()
        if(spdlog_FOUND AND TARGET spdlog::spdlog)
            list(APPEND H5PP_TARGETS spdlog::spdlog)
            if(fmt_FOUND AND TARGET fmt::fmt)
                target_link_libraries(spdlog::spdlog INTERFACE fmt::fmt)
            endif()
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        else()
            message(FATAL_ERROR "Spdlog could not be downloaded and built from source")
        endif()
    endif()

    # Download Eigen3
    if (H5PP_ENABLE_EIGEN3 AND NOT TARGET Eigen3::Eigen)
        set(Eigen3_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for Eigen3 installed by h5pp")
        find_package(Eigen3 3.3.7
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
                )
        if(NOT TARGET Eigen3::Eigen)
            message(STATUS "Eigen3 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            install_package(Eigen3 "${H5PP_DEPS_INSTALL_DIR}" "")
            find_package(Eigen3 3.3.7
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_PACKAGE_REGISTRY
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
                    REQUIRED)
            message(STATUS "Eigen3 installed successfully")
        endif()
        if(Eigen3_FOUND AND TARGET Eigen3::Eigen)
            list(APPEND H5PP_TARGETS Eigen3::Eigen)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        else()
            message(FATAL_ERROR "Eigen3 could not be downloaded and built from source")
        endif()
    endif()


    include(cmake/InstallHDF5.cmake)
    install_hdf5()
endif()
