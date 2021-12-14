function(install_hdf5)
    if(HDF5_FOUND)
        return()
    endif()
    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
        set(INSTALL_SUBDIR hdf5)
    endif()
    if(BUILD_SHARED_LIBS)
        set(HDF5_LINK_TYPE shared)
    else()
        set(HDF5_LINK_TYPE static)
    endif()

    string(JOIN / H5PP_HDF5_INSTALL_DIR  ${H5PP_DEPS_INSTALL_DIR} ${INSTALL_SUBDIR})
    set(HDF5_ROOT   ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for HDF5 installed by h5pp" FORCE)
    set(ZLIB_ROOT   ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for ZLIB installed by h5pp" FORCE)
    set(SZIP_ROOT   ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for SZIP installed by h5pp" FORCE)
    set(LIBAEC_ROOT ${H5PP_HDF5_INSTALL_DIR} CACHE PATH "Default root path for LIBAEC installed by h5pp" FORCE)

    mark_as_advanced(HDF5_LINK_TYPE)
    mark_as_advanced(HDF5_ROOT)
    install_package(zlib
            COMPONENTS static shared
            TARGET_HINTS zlib zlib-static zlib-shared libz
            BUILD_SUBDIR hdf5
            INSTALL_SUBDIR ${INSTALL_SUBDIR})
    install_package(libaec
            COMPONENTS static shared
            TARGET_HINTS sz aec sz-static aec-static sz-shared aec-shared
            BUILD_SUBDIR hdf5
            INSTALL_SUBDIR ${INSTALL_SUBDIR})

    install_package(hdf5 VERSION 1.12
            COMPONENTS C HL ${HDF5_LINK_TYPE}
            FIND_NAME HDF5
            TARGET_HINTS HDF5::HDF5 hdf5::hdf5_hl hdf5::hdf5_hl-${HDF5_LINK_TYPE} hdf5_hl hdf5_hl-${HDF5_LINK_TYPE}
            PATH_SUFFIXES cmake/hdf5 # Needed in vs2019 for some reason
            ${INSTALL_PREFIX_PKGNAME}
            CMAKE_ARGS
            -DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}
            -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
            -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON
            )
    set(PKG_hdf5_TARGET ${PKG_hdf5_TARGET} PARENT_SCOPE)
endfunction()