cmake_minimum_required(VERSION 3.19)

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


    # Download HDF5 (and ZLIB and SZIP)
    if(NOT szip_FOUND OR NOT TARGET szip-static)
        set(szip_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for SZIP installed by h5pp")
        find_package(szip CONFIG NAMES szip sz COMPONENTS static
                PATH_SUFFIXES cmake share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(szip_FOUND OR NOT TARGET szip-static)
            install_package(szip "${H5PP_DEPS_INSTALL_DIR}" "")
            find_package(szip CONFIG NAMES szip sz COMPONENTS static
                    PATH_SUFFIXES cmake share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_PACKAGE_REGISTRY
                    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
                    REQUIRED)
            get_target_property(SZIP_LIBRARY szip-static LOCATION)
            get_target_property(SZIP_INCLUDE_DIR szip-static LOCATION)
        endif()
    endif()

    if(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)
        find_library(ZLIB_LIBRARY NAMES z zlibstatic zlibstaticd zlib zlib1 zlibd HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/lib zlib/bin NO_DEFAULT_PATH)
        find_path(ZLIB_INCLUDE_DIR NAMES zlib.h HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/include NO_DEFAULT_PATH)
        if(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)
            install_package(zlib "${H5PP_DEPS_INSTALL_DIR}" "")
            find_library(ZLIB_LIBRARY NAMES z zlibstatic zlibstaticd zlib zlib1 zlibd HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/lib zlib/bin NO_DEFAULT_PATH REQUIRED)
            find_path(ZLIB_INCLUDE_DIR NAMES zlib.h HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/include NO_DEFAULT_PATH REQUIRED)
        endif()
    endif()

    if(NOT HDF5_FOUND OR NOT TARGET hdf5::all)
        set(HDF5_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for HDF5 installed by h5pp")
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
        # This one uses our own module, but will call the config-mode internally first.
        find_package(HDF5 1.12 COMPONENTS C HL)
        if(NOT HDF5_FOUND OR NOT TARGET hdf5::all)
            message(STATUS "HDF5 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON")
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON")
            list(APPEND H5PP_HDF5_OPTIONS  "-DZLIB_LIBRARY:BOOL=${ZLIB_LIBRARY}")
            list(APPEND H5PP_HDF5_OPTIONS  "-DZLIB_INCLUDE_DIR:BOOL=${ZLIB_INCLUDE_DIR}")
            list(APPEND H5PP_HDF5_OPTIONS  "-DSZIP_LIBRARY:BOOL=${SZIP_LIBRARY}")
            list(APPEND H5PP_HDF5_OPTIONS  "-DSZIP_INCLUDE_DIR:BOOL=${SZIP_INCLUDE_DIR}")
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
            install_package(hdf5 "${H5PP_DEPS_INSTALL_DIR}" "${H5PP_HDF5_OPTIONS}")
            # This one uses our own module, but will call the config-mode internally first.
            find_package(HDF5 1.12 COMPONENTS C HL REQUIRED)
            message(STATUS "hdf5 installed successfully")

        endif()
        if(HDF5_FOUND AND TARGET hdf5::all)
            list(APPEND H5PP_TARGETS hdf5::all)
            target_link_libraries(deps INTERFACE hdf5::all)
        else()
            message(FATAL_ERROR "HDF5 could not be downloaded and built from source")
        endif()
    endif()
endif()
