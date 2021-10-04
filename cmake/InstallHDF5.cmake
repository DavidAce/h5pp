function(install_hdf5)
    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
    endif()
    if(BUILD_SHARED_LIBS)
        set(HDF5_LINK_TYPE shared)
    else()
        set(HDF5_LINK_TYPE static)
    endif()
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(HDF5_ROOT ${H5PP_DEPS_INSTALL_DIR}/hdf5 CACHE PATH "Default root path for HDF5 installed by h5pp" FORCE)
        set(ZLIB_ROOT ${H5PP_DEPS_INSTALL_DIR}/zlib CACHE PATH "Default root path for ZLIB installed by h5pp" FORCE)
    else()
        set(HDF5_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for HDF5 installed by h5pp" FORCE)
        set(ZLIB_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for ZLIB installed by h5pp" FORCE)
    endif()

    mark_as_advanced(HDF5_LINK_TYPE)
    mark_as_advanced(HDF5_ROOT)
    mark_as_advanced(ZLIB_ROOT)

    # Download HDF5 (and ZLIB and SZIP)
    install_package(szip
            NAMES szip sz
            FIND_NAME SZIP
            COMPONENTS static shared
            PATH_SUFFIXES cmake share/cmake # Fixes bug in CMake 3.20.2 not generating search paths
            ${INSTALL_PREFIX_PKGNAME}
            )
    install_package(zlib
            MODULE
            FIND_NAME ZLIB
            LIBRARY_NAMES
            z zlib zdll zlib1 zlibstatic # Release names
            zd zlibd zdlld zlibd1 zlib1d zlibstaticd # Debug names
            ${INSTALL_PREFIX_PKGNAME})

    install_package(hdf5 VERSION 1.12
            COMPONENTS C HL ${HDF5_LINK_TYPE}
            FIND_NAME HDF5
            TARGET_HINTS hdf5::hdf5_hl hdf5::hdf5_hl-${HDF5_LINK_TYPE} hdf5_hl hdf5_hl-${HDF5_LINK_TYPE}
            PATH_SUFFIXES cmake/hdf5 # Needed in vs2019 for some reason
            ${INSTALL_PREFIX_PKGNAME}
            CMAKE_ARGS
            -DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}
            -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
            -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON
            )
    set(PKG_hdf5_TARGET ${PKG_hdf5_TARGET} ${PKG_zlib_TARGET} ${PKG_szip_TARGET} PARENT_SCOPE)
    set(PKG_zlib_TARGET ${PKG_zlib_TARGET} PARENT_SCOPE)
    set(PKG_szip_TARGET ${PKG_szip_TARGET} PARENT_SCOPE)
endfunction()