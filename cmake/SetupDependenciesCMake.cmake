cmake_minimum_required(VERSION 3.15)

macro(unset_NOT_FOUND pkg)
    if(NOT ${pkg}_FOUND)
        unset(${pkg}_FOUND)
        unset(${pkg}_FOUND CACHE)
    endif()
endmacro()


if(H5PP_PACKAGE_MANAGER MATCHES "cmake")
    unset_NOT_FOUND(fmt)
    unset_NOT_FOUND(spdlog)
    unset_NOT_FOUND(Eigen3)
    unset_NOT_FOUND(HDF5)

    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
    endif()


    # Install all the depeendencies
    if(H5PP_ENABLE_FMT)
        install_package(fmt VERSION 8.1.1 ${INSTALL_PREFIX_PKGNAME})
    endif()
    if(H5PP_ENABLE_SPDLOG)
        install_package(spdlog VERSION 1.10.0 ${INSTALL_PREFIX_PKGNAME}
                        DEPENDS fmt::fmt
                        CMAKE_ARGS
                            -DSPDLOG_FMT_EXTERNAL:BOOL=ON
                            -Dfmt_ROOT:PATH=${H5PP_DEPS_INSTALL_DIR})
    endif()
    if(H5PP_ENABLE_EIGEN3)
        install_package(Eigen3 VERSION 3.4.0 TARGET_NAME Eigen3::Eigen ${INSTALL_PREFIX_PKGNAME})
    endif()


    include(cmake/InstallHDF5.cmake)
    install_hdf5()

    # Link to h5pp dependencies
    target_link_libraries(deps INTERFACE ${PKG_fmt_TARGET} ${PKG_spdlog_TARGET} ${PKG_Eigen3_TARGET} ${PKG_hdf5_TARGET})

endif()
