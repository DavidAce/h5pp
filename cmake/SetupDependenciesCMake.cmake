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
        find_package(fmt 6.2.1
                HINTS ${H5PP_DEPS_INSTALL_DIR}
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(NOT fmt_FOUND OR NOT TARGET fmt::fmt)
            message(STATUS "fmt will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            install_package(fmt  "${H5PP_DEPS_INSTALL_DIR}" "${FMT_CMAKE_OPTIONS}")
            find_package(fmt 6.2.1
                    HINTS ${H5PP_DEPS_INSTALL_DIR}
                    NO_DEFAULT_PATH
                    REQUIRED)
        endif()
        if(TARGET fmt::fmt AND fmt_FOUND)
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
        if(NOT spdlog_FOUND OR NOT TARGET spdlog::spdlog)
            message(STATUS "Spdlog will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            if(fmt_FOUND AND TARGET fmt::fmt)
                get_target_property(FMT_INCLUDE_DIR  fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
                get_filename_component(fmt_ROOT ${FMT_INCLUDE_DIR}/.. ABSOLUTE)
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-DSPDLOG_FMT_EXTERNAL:BOOL=ON")
                list(APPEND SPDLOG_CMAKE_OPTIONS  "-Dfmt_ROOT:PATH=${fmt_ROOT}")
            endif()
            install_package(spdlog  "${H5PP_DEPS_INSTALL_DIR}" "${SPDLOG_CMAKE_OPTIONS}")
            find_package(spdlog 1.8.5
                    HINTS ${H5PP_DEPS_INSTALL_DIR}
                    NO_DEFAULT_PATH
                    REQUIRED)
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
        find_package(Eigen3 3.3.7
                HINTS ${H5PP_DEPS_INSTALL_DIR} ${EIGEN3_INCLUDE_DIR}
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
                )
        if(NOT TARGET Eigen3::Eigen)
            message(STATUS "Eigen3 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            install_package(Eigen3 "${H5PP_DEPS_INSTALL_DIR}" "")
            find_package(Eigen3 3.3.7
                    HINTS ${H5PP_DEPS_INSTALL_DIR}
                    NO_DEFAULT_PATH
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
    if(NOT TARGET hdf5::all)
        list(INSERT HDF5_ROOT 0 ${H5PP_DEPS_INSTALL_DIR})
#        set(CMAKE_FIND_DEBUG_MODE ON)
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
        if(NOT HDF5_FOUND OR NOT TARGET hdf5::all)
            message(STATUS "HDF5 will be installed into ${H5PP_DEPS_INSTALL_DIR}")
            find_library(SZIP_LIBRARY NAMES sz szip     HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec lib aec/lib NO_DEFAULT_PATH)
            find_library(AEC_LIBRARY  NAMES aec         HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec lib aec/lib NO_DEFAULT_PATH)
            find_library(ZLIB_LIBRARY NAMES  z          HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib lib zlib/lib NO_DEFAULT_PATH)
            find_path(SZIP_ROOT NAMES include/szlib.h   HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec NO_DEFAULT_PATH)
            find_path(SZIP_INCLUDE_DIR NAMES szlib.h    HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec/include NO_DEFAULT_PATH)
            find_path(ZLIB_ROOT NAMES include/zlib.h    HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib NO_DEFAULT_PATH)
            find_path(ZLIB_INCLUDE_DIR NAMES zlib.h     HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/include NO_DEFAULT_PATH)
            if(NOT SZIP_LIBRARY OR NOT AEC_LIBRARY)
                install_package(aec "${H5PP_DEPS_INSTALL_DIR}" "")
                find_library(SZIP_LIBRARY NAMES sz szip     HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec lib aec/lib NO_DEFAULT_PATH REQUIRED)
                find_library(AEC_LIBRARY  NAMES aec         HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec lib aec/lib NO_DEFAULT_PATH REQUIRED)
                find_path(SZIP_ROOT NAMES include/szlib.h   HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec NO_DEFAULT_PATH REQUIRED)
                find_path(SZIP_INCLUDE_DIR NAMES szlib.h    HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES aec/include NO_DEFAULT_PATH REQUIRED)
            endif()
            if(NOT ZLIB_LIBRARY)
                install_package(zlib "${H5PP_DEPS_INSTALL_DIR}" "")
                find_library(ZLIB_LIBRARY NAMES  z  HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib lib zlib/lib NO_DEFAULT_PATH REQUIRED)
                find_path(ZLIB_ROOT NAMES include/zlib.h HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib NO_DEFAULT_PATH REQUIRED)
                find_path(ZLIB_INCLUDE_DIR NAMES zlib.h HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/include NO_DEFAULT_PATH REQUIRED)
            endif()
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
            install_package(hdf5 "${H5PP_DEPS_INSTALL_DIR}" "${H5PP_HDF5_OPTIONS}")
            # This one uses our own module though, but will call the config-mode internally first.
            find_package(HDF5 1.12 COMPONENTS C HL REQUIRED)
            message(STATUS "hdf5 installed successfully")
        endif()
#        set(CMAKE_FIND_DEBUG_MODE OFF)
        if(HDF5_FOUND AND TARGET hdf5::all)
            list(APPEND H5PP_TARGETS hdf5::all)
            target_link_libraries(deps INTERFACE hdf5::all)
        else()
            message(FATAL_ERROR "HDF5 could not be downloaded and built from source")
        endif()
    endif()
endif()
