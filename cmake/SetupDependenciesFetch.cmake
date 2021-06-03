if(H5PP_PACKAGE_MANAGER MATCHES "fetch")


    if(NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 17)
    endif()
    if(NOT CMAKE_CXX_STANDARD_REQUIRED)
        set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
    endif()
    if(NOT CMAKE_CXX_EXTENSIONS)
        set(CMAKE_CXX_EXTENSIONS FALSE)
    endif()


    if(H5PP_ENABLE_EIGEN3)
        if(NOT TARGET Eigen3::Eigen)
            include(cmake/fetch/FetchEigen3.cmake)
        endif()
        if(TARGET Eigen3::Eigen)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        else()
            message(FATAL_ERROR "Failed FetchContent: Eigen3")
        endif()
    endif()

    if(H5PP_ENABLE_FMT)
        if(NOT TARGET fmt::fmt)
            include(cmake/fetch/Fetchfmt.cmake)
        endif()
        if(TARGET fmt::fmt)
            target_link_libraries(deps INTERFACE fmt::fmt)
        else()
            message(FATAL_ERROR "Failed FetchContent: fmt")
        endif()
    endif()

    if(H5PP_ENABLE_SPDLOG)
        if(NOT TARGET spdlog::spdlog)
            include(cmake/fetch/Fetchspdlog.cmake)
        endif()
        if(TARGET spdlog::spdlog)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        else()
            message(FATAL_ERROR "Failed FetchContent: spdlog")
        endif()
    endif()


    # Download HDF5 (and ZLIB and SZIP)
    include(cmake/InstallPackage.cmake)
    if(NOT szip_FOUND OR NOT TARGET szip-static)
        set(SZIP_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for SZIP installed by h5pp")
        find_package(SZIP CONFIG NAMES szip sz COMPONENTS static
                PATH_SUFFIXES share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_PACKAGE_REGISTRY
                NO_CMAKE_SYSTEM_PATH
                NO_CMAKE_SYSTEM_PACKAGE_REGISTRY)
        if(szip_FOUND OR NOT TARGET szip-static)
            install_package(szip "${H5PP_DEPS_INSTALL_DIR}" "")
            find_package(SZIP CONFIG NAMES szip sz COMPONENTS static
                    PATH_SUFFIXES share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
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
        find_library(ZLIB_LIBRARY NAMES z HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/lib NO_DEFAULT_PATH)
        find_path(ZLIB_INCLUDE_DIR NAMES zlib.h HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/include NO_DEFAULT_PATH)
        if(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)
            install_package(zlib "${H5PP_DEPS_INSTALL_DIR}" "")
            find_library(ZLIB_LIBRARY NAMES z HINTS ${H5PP_DEPS_INSTALL_DIR} PATH_SUFFIXES zlib/lib NO_DEFAULT_PATH REQUIRED)
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