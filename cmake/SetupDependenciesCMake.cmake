cmake_minimum_required(VERSION 3.14)

if(H5PP_PACKAGE_MANAGER MATCHES "cmake")

    # Download fmt
    if (H5PP_ENABLE_FMT AND NOT TARGET fmt::fmt)
        # fmt is a dependency of spdlog
        # We fetch it here to get the latest version and to make sure we use the
        # compile library and avoid compile-time overhead in projects consuming h5pp.
        # Note that spdlog may already have been found in if H5PP_PACKAGE_MANAGER=find|cmake
        # then we can assume that spdlog already knows how and where to get fmt.
        find_package(fmt 6.2.1
                HINTS ${H5PP_DEPS_INSTALL_DIR}
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(NOT TARGET fmt::fmt)
            message(STATUS "fmt will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            list(APPEND FMT_CMAKE_OPTIONS  "-DFMT_TEST:BOOL=OFF")
            list(APPEND FMT_CMAKE_OPTIONS  "-DFMT_DOC:BOOL=OFF")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(fmt  "${H5PP_DEPS_INSTALL_DIR}" "${FMT_CMAKE_OPTIONS}")
            find_package(fmt 6.2.1
                    HINTS ${H5PP_DEPS_INSTALL_DIR}
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
                HINTS ${H5PP_DEPS_INSTALL_DIR}
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(NOT TARGET spdlog::spdlog)
            message(STATUS "Spdlog will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            if(TARGET fmt::fmt)
                get_target_property(FMT_INC  fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
                get_target_property(FMT_LOC  fmt::fmt LOCATION)
                get_filename_component(fmt_ROOT ${FMT_INC}/.. ABSOLUTE)
                mark_as_advanced(FMT_LOC)
                mark_as_advanced(FMT_INC)
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-DSPDLOG_FMT_EXTERNAL:BOOL=ON")
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-Dfmt_ROOT:PATH=${fmt_ROOT}")
            endif()
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(spdlog  "${H5PP_DEPS_INSTALL_DIR}" "${SPDLOG_CMAKE_OPTIONS}")
            find_package(spdlog 1.3
                    HINTS ${H5PP_DEPS_INSTALL_DIR}
                    NO_DEFAULT_PATH
                    REQUIRED)
            if(TARGET spdlog::spdlog)
                message(STATUS "spdlog installed successfully")
            endif()
        endif()
        if(TARGET spdlog::spdlog)
            list(APPEND H5PP_TARGETS spdlog::spdlog)
            if(TARGET fmt::fmt)
                target_link_libraries(spdlog::spdlog INTERFACE fmt::fmt)
            else()
                message(FATAL_ERROR "Missing target fmt::fmt is required for Spdlog")
            endif()
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        else()
            message(FATAL_ERROR "Spdlog could not be downloaded and built from source")
        endif()
    endif()

    # Download Eigen3
    if (H5PP_ENABLE_EIGEN3 AND NOT TARGET Eigen3::Eigen)
        find_package(Eigen3 3.3.7
                HINTS ${H5PP_DEPS_INSTALL_DIR} ${EIGEN3_INCLUDE_DIR}
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
                )
        if(NOT TARGET Eigen3::Eigen)
            message(STATUS "Eigen3 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(Eigen3 "${H5PP_DEPS_INSTALL_DIR}" "")
            find_package(Eigen3 3.3.7
                    HINTS ${H5PP_DEPS_INSTALL_DIR}
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
    if(NOT TARGET hdf5::all)
        list(APPEND HDF5_ROOT ${H5PP_DEPS_INSTALL_DIR})
        set(HDF5_NO_SYSTEM_ENVIRONMENT_PATH ON)
        set(HDF5_NO_CMAKE_PACKAGE_REGISTRY ON)
        set(HDF5_NO_CMAKE_SYSTEM_PATH ON)
        set(HDF5_NO_CMAKE_SYSTEM_PACKAGE_REGISTRY ON)
        set(HDF5_FIND_VERBOSE OFF)
        set(HDF5_FIND_DEBUG OFF)
        mark_as_advanced(HDF5_NO_SYSTEM_ENVIRONMENT_PATH)
        mark_as_advanced(HDF5_NO_CMAKE_PACKAGE_REGISTRY)
        mark_as_advanced(HDF5_NO_CMAKE_SYSTEM_PATH)
        mark_as_advanced(HDF5_NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        mark_as_advanced(HDF5_FIND_VERBOSE)
        mark_as_advanced(HDF5_FIND_DEBUG)


        find_package(HDF5 1.8 COMPONENTS C HL)
        if(NOT TARGET hdf5::all)
            message(STATUS "HDF5 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
            build_dependency(hdf5 "${H5PP_DEPS_INSTALL_DIR}" "${H5PP_HDF5_OPTIONS}")
            # This one uses our own module though, but will call the config-mode internally first.
            find_package(HDF5 1.12 COMPONENTS C HL REQUIRED)
            if(TARGET hdf5::all)
                message(STATUS "hdf5 installed successfully")
            endif()
        endif()
        if(TARGET hdf5::all)
            list(APPEND H5PP_TARGETS hdf5::all)
            target_link_libraries(deps INTERFACE hdf5::all)
        else()
            message(FATAL_ERROR "HDF5 could not be downloaded and built from source")
        endif()
    endif()
endif()
