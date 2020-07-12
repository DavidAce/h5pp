if(H5PP_DOWNLOAD_METHOD MATCHES "fetch")
    # Here we use find_package in config-mode, intended to find <packagename>Config.cmake
    # that is bundled with source installs of these packages.

    # Download fmt
    if (H5PP_ENABLE_SPDLOG AND NOT TARGET fmt::fmt AND NOT TARGET spdlog::spdlog)
        # fmt is a dependency of spdlog
        # We fetch it here to get the latest version and to make sure we use the
        # compile library and avoid compile-time overhead in projects consuming h5pp.
        # Note that spdlog may already have been found in if H5PP_DOWNLOAD_METHOD=find|fetch
        # then we can assume that spdlog already knows how and where to get fmt.
        find_package(fmt 7.0.0
                HINTS ${CMAKE_INSTALL_PREFIX}
                NO_DEFAULT_PATH)
        if(NOT TARGET fmt::fmt)
            message(STATUS "fmt will be installed into ${CMAKE_INSTALL_PREFIX}")
            list(APPEND FMT_CMAKE_OPTIONS  "-DFMT_TEST:BOOL=OFF")
            list(APPEND FMT_CMAKE_OPTIONS  "-DFMT_DOC:BOOL=OFF")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(fmt  "${CMAKE_INSTALL_PREFIX}" "")
            find_package(fmt 7.0.0
                    HINTS ${CMAKE_INSTALL_PREFIX}
                    NO_DEFAULT_PATH
                    REQUIRED)
            if(TARGET fmt::fmt)
                message(STATUS "fmt installed successfully")
            endif()
        endif()
        if(TARGET fmt::fmt)
            list(APPEND H5PP_TARGETS fmt::fmt)
        else()
            message(FATAL_ERROR "fmt could not be downloaded and built from source")
        endif()
    endif()


    # Download spdlog
    if (H5PP_ENABLE_SPDLOG AND NOT TARGET spdlog::spdlog)
        find_package(spdlog 1.3
                HINTS ${CMAKE_INSTALL_PREFIX}
                NO_DEFAULT_PATH)
        if(NOT TARGET spdlog::spdlog)
            message(STATUS "Spdlog will be installed into ${CMAKE_INSTALL_PREFIX}")
            if(TARGET fmt::fmt)
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-DSPDLOG_FMT_EXTERNAL:BOOL=ON")
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-Dfmt_ROOT:PATH=${CMAKE_INSTALL_PREFIX}/fmt")
            endif()
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(spdlog  "${CMAKE_INSTALL_PREFIX}" "${SPDLOG_CMAKE_OPTIONS}")
            find_package(spdlog 1.3
                    HINTS ${CMAKE_INSTALL_PREFIX}
                    NO_DEFAULT_PATH
                    REQUIRED)
            if(TARGET spdlog::spdlog)
                message(STATUS "spdlog installed successfully")
            endif()
        endif()
        if(TARGET spdlog::spdlog)
            list(APPEND H5PP_TARGETS spdlog::spdlog)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
            if(TARGET fmt::fmt)
                target_link_libraries(spdlog::spdlog INTERFACE fmt::fmt)
            else()
                message(FATAL_ERROR "Missing target fmt::fmt is required for Spdlog")
            endif()
        else()
            message(FATAL_ERROR "Spdlog could not be downloaded and built from source")
        endif()
    endif()

    # Download Eigen3
    if (H5PP_ENABLE_EIGEN3 AND NOT TARGET Eigen3::Eigen)
        find_package(Eigen3 3.3.7
                HINTS ${CMAKE_INSTALL_PREFIX} ${EIGEN3_DIRECTORY_HINTS} ${EIGEN3_INCLUDE_DIR}
                NO_DEFAULT_PATH)
        if(NOT TARGET Eigen3::Eigen)
            message(STATUS "Eigen3 will be installed into ${CMAKE_INSTALL_PREFIX}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(Eigen3 "${CMAKE_INSTALL_PREFIX}" "")
            find_package(Eigen3 3.3.7
                    HINTS ${CMAKE_INSTALL_PREFIX}
                    NO_DEFAULT_PATH
                    REQUIRED)
            if(TARGET Eigen3::Eigen)
                message(STATUS "Eigen3 installed successfully")
            endif()
        endif()
        if(TARGET Eigen3::Eigen)
            list(APPEND H5PP_TARGETS Eigen3::Eigen)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        else()
            message(FATAL_ERROR "Eigen3 could not be downloaded and built from source")
        endif()
    endif()


    # Download HDF5
    if(NOT TARGET hdf5::hdf5)
        set(HDF5_ROOT ${CMAKE_INSTALL_PREFIX})
        set(HDF5_NO_DEFAULT_PATH ON)
        find_package(HDF5 1.8 COMPONENTS C HL)
        if(NOT TARGET hdf5::hdf5)
            message(STATUS "HDF5 will be installed into ${CMAKE_INSTALL_PREFIX}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
            build_dependency(hdf5 "${CMAKE_INSTALL_PREFIX}" "${H5PP_HDF5_OPTIONS}")
            # This one uses our own module though, but will call the config-mode internally first.
            find_package(HDF5 1.8 COMPONENTS C HL REQUIRED)
            if(TARGET hdf5::hdf5)
                message(STATUS "hdf5 installed successfully")
            endif()
        endif()
        if(TARGET hdf5::hdf5)
            list(APPEND H5PP_TARGETS hdf5::hdf5)
            target_link_libraries(deps INTERFACE hdf5::hdf5)
        else()
            message(FATAL_ERROR "HDF5 could not be downloaded and built from source")
        endif()
    endif()
endif()
