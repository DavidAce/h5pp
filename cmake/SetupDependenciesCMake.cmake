cmake_minimum_required(VERSION 3.15)

if(H5PP_PACKAGE_MANAGER MATCHES "cmake")
    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
    endif()


    # Install all the depeendencies
    if(H5PP_ENABLE_FMT)
        install_package(fmt VERSION 8.0.1 ${INSTALL_PREFIX_PKGNAME})
    endif()
    if(H5PP_ENABLE_SPDLOG)
        install_package(spdlog VERSION 1.9.2 ${INSTALL_PREFIX_PKGNAME}
                        DEPENDS fmt::fmt
                        CMAKE_ARGS
                            -DSPDLOG_FMT_EXTERNAL:BOOL=ON
                            -Dfmt_ROOT:PATH=${PKG_INSTALL_DIR})
    endif()
    if(H5PP_ENABLE_EIGEN3)
        install_package(Eigen3 VERSION 3.4.0 TARGET_NAME Eigen3::Eigen ${INSTALL_PREFIX_PKGNAME})
    endif()


    include(cmake/InstallHDF5.cmake)
    install_hdf5()

    # Link to h5pp dependencies
    target_link_libraries(deps INTERFACE ${fmt_TARGET} ${spdlog_TARGET} ${Eigen3_TARGET} ${hdf5_TARGET})

endif()
