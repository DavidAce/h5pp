
if(NOT TARGET hdf5::hdf5)
    include(cmake-modules/FindPackageHDF5.cmake)

    if(HDF5_FOUND AND TARGET hdf5::hdf5)
#        message(STATUS "hdf5 found")

    elseif (DOWNLOAD_MISSING)
        message(STATUS "HDF5 will be installed into ${CMAKE_INSTALL_PREFIX}")
        include(${PROJECT_SOURCE_DIR}/cmake-modules/BuildDependency.cmake)
        build_dependency(hdf5 "")
        include(${PROJECT_SOURCE_DIR}/cmake-modules/FindPackageHDF5.cmake)
        if(HDF5_FOUND AND TARGET hdf5::hdf5)
            message(STATUS "hdf5 installed successfully: ${HDF5_BUILD_DIR} ${HDF5_CXX_INCLUDE_DIRS} ${HDF5_hdf5_LIBRARY}")
        else()
            message(STATUS "config_result: ${config_result}")
            message(STATUS "build_result: ${build_result}")
            message(FATAL_ERROR "hdf5 could not be downloaded.")
        endif()
    else()
        message("WARNING: Dependency HDF5 not found and DOWNLOAD_MISSING is OFF. Build will fail.")
    endif()

endif()
