#include(${PROJECT_SOURCE_DIR}/cmake/FindPackageHDF5.cmake)

if(NOT TARGET hdf5::hdf5)
    set(HDF5_PREFER_PARALLEL ON)
    find_package(HDF5 1.8 COMPONENTS C HL REQUIRED)
    if(TARGET hdf5::hdf5)
        set(HDF5_FOUND TRUE)
    elseif (H5PP_DOWNLOAD_METHOD MATCHES "native")
        message(STATUS "HDF5 will be installed into ${CMAKE_INSTALL_PREFIX}")
        include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
        list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
        build_dependency(hdf5 "${hdf5_install_prefix}" "${H5PP_HDF5_OPTIONS}")
        set(HDF5_ROOT ${hdf5_install_prefix})
        find_package_hdf5()
        if(TARGET hdf5::hdf5)
            message(STATUS "hdf5 installed successfully: ${HDF5_BUILD_DIR} ${HDF5_CXX_INCLUDE_DIRS} ${HDF5_hdf5_LIBRARY}")
        else()
            message(FATAL_ERROR "hdf5 could not be downloaded.")
        endif()
    else()
        message("WARNING: Dependency HDF5 not found in your system. Set H5PP_DOWNLOAD_METHOD to one of 'conan|native'")
    endif()

endif()
