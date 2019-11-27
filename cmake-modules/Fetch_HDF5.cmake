
if(NOT TARGET hdf5::hdf5)
    enable_language(C)
    include(cmake-modules/FindPackageHDF5.cmake)

    if(HDF5_FOUND AND TARGET hdf5::hdf5)
        message(STATUS "hdf5 found")
#        include(cmake-modules/PrintTargetProperties.cmake)
#        print_target_properties(hdf5::hdf5)
    elseif (DOWNLOAD_MISSING)
        message(STATUS "HDF5 will be installed into ${H5PP_INSTALL_DIR_THIRD_PARTY}/hdf5")
        include(cmake-modules/BuildThirdParty.cmake)
        build_third_party(
                "hdf5"
                "${H5PP_CONFIG_DIR_THIRD_PARTY}"
                "${H5PP_BUILD_DIR_THIRD_PARTY}"
                "${H5PP_INSTALL_DIR_THIRD_PARTY}"
                "-DCMAKE_BUILD_TYPE=Release"
#                "-DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} -DHDF5_ENABLE_PARALLEL:BOOL=OFF"
        )
#        set(ENV{PKG_CONFIG_PATH}  "$ENV{PKG_CONFIG_PATH}:${H5PP_INSTALL_DIR_THIRD_PARTY}/hdf5/lib/pkgconfig")

#        find_package(PkgConfig REQUIRED)
#        pkg_search_module(HDF5 REQUIRED hdf5-1.10.3.pc)  # this looks for opencv.pc file
        include(cmake-modules/FindPackageHDF5.cmake)
        if(HDF5_FOUND AND TARGET hdf5::hdf5)
            message(STATUS "hdf5 installed successfully: ${HDF5_BUILD_DIR} ${HDF5_CXX_INCLUDE_DIRS} ${HDF5_hdf5_LIBRARY}")
#            include(cmake-modules/PrintTargetProperties.cmake)
#            print_target_properties(hdf5::hdf5)
        else()
            message(STATUS "config_result: ${config_result}")
            message(STATUS "build_result: ${build_result}")
            message(FATAL_ERROR "hdf5 could not be downloaded.")
        endif()


    else()
        message("WARNING: Dependency HDF5 not found and DOWNLOAD_MISSING is OFF. Build will fail.")
    endif()

endif()
