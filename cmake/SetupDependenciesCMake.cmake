cmake_minimum_required(VERSION 3.15)

if(H5PP_PACKAGE_MANAGER MATCHES "cmake")
    include(cmake/InstallPackage.cmake)
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(INSTALL_PREFIX_PKGNAME INSTALL_PREFIX_PKGNAME)
    endif()
    # Setup build/find options for dependencies
    list(APPEND H5PP_spdlog_ARGS  "-DSPDLOG_FMT_EXTERNAL:BOOL=ON")
    list(APPEND H5PP_spdlog_ARGS  "-Dfmt_ROOT:PATH=${PKG_INSTALL_DIR}")
    list(APPEND H5PP_HDF5_ARGS    "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
    list(APPEND H5PP_HDF5_ARGS    "-DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON")
    list(APPEND H5PP_HDF5_ARGS    "-DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON")
    list(APPEND H5PP_HDF5_ARGS    "-DCMAKE_FIND_DEBUG_MODE:BOOL=ON")
    set(HDF5_NO_DEFAULT_PATH ON)
    if(BUILD_SHARED_LIBS)
        set(HDF5_LINK_TYPE shared)
    else()
        set(HDF5_LINK_TYPE static)
    endif()
    if(H5PP_PREFIX_ADD_PKGNAME)
        set(ZLIB_ROOT ${H5PP_DEPS_INSTALL_DIR}/zlib CACHE PATH "Default root path for ZLIB installed by h5pp" FORCE)
        list(APPEND H5PP_HDF5_ARGS  "-DZLIB_ROOT:PATH=${H5PP_DEPS_INSTALL_DIR}/zlib")
    else()
        set(ZLIB_ROOT ${H5PP_DEPS_INSTALL_DIR} CACHE PATH "Default root path for ZLIB installed by h5pp" FORCE)
        list(APPEND H5PP_HDF5_ARGS  "-DZLIB_ROOT:PATH=${H5PP_DEPS_INSTALL_DIR}")
    endif()

    mark_as_advanced(H5PP_spdlog_ARGS)
    mark_as_advanced(H5PP_HDF5_ARGS)
    mark_as_advanced(HDF5_NO_DEFAULT_PATH)
    mark_as_advanced(HDF5_LINK_TYPE)
    mark_as_advanced(ZLIB_ROOT)


    # Install all the depeendencies
    if(H5PP_ENABLE_FMT)
        install_package(fmt VERSION 8.0.1 ${INSTALL_PREFIX_PKGNAME})
    endif()
    if(H5PP_ENABLE_SPDLOG)
        install_package(spdlog VERSION 1.9.2 DEPENDS fmt::fmt CMAKE_ARGS ${H5PP_spdlog_ARGS} ${INSTALL_PREFIX_PKGNAME})
    endif()
    if(H5PP_ENABLE_EIGEN3)
        install_package(Eigen3 VERSION 3.4.0 TARGET_NAME Eigen3::Eigen ${INSTALL_PREFIX_PKGNAME})
    endif()
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
            CMAKE_ARGS ${H5PP_HDF5_ARGS}
            ${INSTALL_PREFIX_PKGNAME})

    list(APPEND H5PP_TARGETS ${fmt_TARGET} ${spdlog_TARGET} ${Eigen3_TARGET} ${hdf5_TARGET})

    foreach(tgt ${H5PP_TARGETS})
        if(NOT TARGET ${tgt})
            message(FATAL_ERROR "Undefined target: ${tgt}")
        endif()
    endforeach()


    # Link to h5pp dependencies
    target_link_libraries(deps INTERFACE ${H5PP_TARGETS})
endif()
