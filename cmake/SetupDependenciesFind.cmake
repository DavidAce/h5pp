if(H5PP_PACKAGE_MANAGER MATCHES "find")
    if(H5PP_PACKAGE_MANAGER STREQUAL "find")
        set(REQUIRED REQUIRED)
    endif()

    if(NOT "${CMAKE_BINARY_DIR}" IN_LIST CMAKE_MODULE_PATH)
        list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
        list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/conan)
    endif()

    # Start finding the dependencies
    if(H5PP_ENABLE_EIGEN3 AND NOT Eigen3_FOUND )
        find_package(Eigen3 3.3 ${REQUIRED})
        if(Eigen3_FOUND)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        endif()
    endif()

    if(H5PP_ENABLE_FMT AND NOT fmt_FOUND)
        find_package(fmt 6.1.2 ${REQUIRED})
        if(fmt_FOUND)
            target_link_libraries(deps INTERFACE fmt::fmt)
        endif()
    endif()
    if(H5PP_ENABLE_SPDLOG AND NOT spdlog_FOUND)
        find_package(spdlog 1.5.0 ${REQUIRED})
        if(spdlog_FOUND AND TARGET spdlog AND NOT TARGET spdlog::spdlog)
            add_library(spdlog::spdlog ALIAS spdlog)
        endif()
        if(spdlog_FOUND)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        endif()
    endif()

    if (NOT HDF5_FOUND)
        # Note that the call below defaults to FindHDF5.cmake bundled with h5pp,
        # because cmake/modules has been added to CMAKE_MODULE_PATH in cmake/SetupPaths.cmake
        # Also, we dont impose any version requirement here: h5pp is compatible with 1.8 to 1.13, which
        # is most of the packages found in the wild.
        find_package(HDF5 COMPONENTS C HL ${REQUIRED})
        if(HDF5_FOUND)
            include(cmake/HDF5TargetUtils.cmake)
            h5pp_get_modern_hdf5_target_name()
            target_link_libraries(deps INTERFACE HDF5::HDF5)
        endif()
    endif()
endif()