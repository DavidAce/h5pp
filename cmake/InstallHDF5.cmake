function(install_hdf5)
    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
        set(INSTALL_SUBDIR hdf5)
    endif()
    if(NOT BUILD_SHARED_LIBS OR HDF5_USE_STATIC_LIBRARIES)
        set(HDF5_LINK_TYPE static)
    else()
        set(HDF5_LINK_TYPE shared)
    endif()

    string(JOIN / H5PP_HDF5_INSTALL_DIR  ${H5PP_DEPS_INSTALL_DIR} ${INSTALL_SUBDIR})
    set(HDF5_ROOT   ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for HDF5 installed by h5pp" FORCE)
    set(ZLIB_ROOT   ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for ZLIB installed by h5pp" FORCE)
    set(SZIP_ROOT   ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for SZIP installed by h5pp" FORCE)

    mark_as_advanced(HDF5_LINK_TYPE)
    mark_as_advanced(HDF5_ROOT)

    install_package(zlib
            MODULE
            FIND_NAME ZLIB
            LINK_TYPE static
            BUILD_SUBDIR hdf5
            LIBRARY_NAMES
                z zlib zdll zlib1 zlibstatic # Release names
                zd zlibd zdlld zlibd1 zlib1d zlibstaticd # Debug names
            INSTALL_SUBDIR ${INSTALL_SUBDIR})

    install_package(szip
            COMPONENTS static
            CONFIG
            TARGET_HINTS szip
            LINK_TYPE static
            BUILD_SUBDIR hdf5
            PATH_SUFFIXES cmake share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
            INSTALL_SUBDIR ${INSTALL_SUBDIR})

    install_package(hdf5 VERSION 1.13.2
            COMPONENTS C HL ${HDF5_LINK_TYPE}
            CONFIG
            FIND_NAME HDF5
            TARGET_HINTS hdf5::hdf5_hl hdf5_hl
            LINK_TYPE ${HDF5_LINK_TYPE}
            PATH_SUFFIXES cmake/hdf5 # Needed in vs2019 for some reason
            ${INSTALL_PREFIX_PKGNAME}
            DEPENDS
            CMAKE_ARGS
            -DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}
            -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
            -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON
            )
    include(cmake/HDF5TargetUtils.cmake)
    h5pp_get_modern_hdf5_target_name()
    target_link_libraries(HDF5::HDF5 INTERFACE ${PKG_zlib_TARGET} ${PKG_szip_TARGET})
    set(PKG_hdf5_TARGET HDF5::HDF5 PARENT_SCOPE)
endfunction()