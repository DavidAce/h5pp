
unset(PKG_IS_RUNNING CACHE) # Remove from cache when this file is included

function(pkg_install_dependencies  package_name)
    if(NOT PKG_INSTALL_SUCCESS AND NOT PKG_IS_RUNNING)
        message(STATUS "pkg_install_dependencies running with package_name: ${package_name}")
        unset(PKG_INSTALL_SUCCESS CACHE)
        set(PKG_IS_RUNNING TRUE CACHE INTERNAL "" FORCE)
        include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PKGInstall.cmake)

        if(H5PP_ENABLE_EIGEN3)
            pkg_install(Eigen3)
        endif()
        if(H5PP_ENABLE_FMT)
            pkg_install(fmt)
        endif()
        if(H5PP_ENABLE_SPDLOG)
            pkg_install(spdlog
                    CMAKE_ARGS
                    -DSPDLOG_FMT_EXTERNAL:BOOL=TRUE
                    -Dfmt_ROOT:PATH=${CMAKE_PREFIX_PATH}
            )
        endif()

        pkg_install(zlib)
#        pkg_install(szip)
        pkg_install(libaec)
        pkg_install(hdf5
            CMAKE_ARGS
            -DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}
            -DHDF5_ENABLE_Z_LIB_SUPPORT:BOOL=ON
            -DHDF5_ENABLE_SZIP_SUPPORT:BOOL=ON
        )

        set(PKG_INSTALL_SUCCESS TRUE CACHE BOOL "PKG dependency install has been invoked and was successful")
        set(PKG_IS_RUNNING FALSE CACHE INTERNAL "" FORCE)
    endif()

    find_package(${ARGN} BYPASS_PROVIDER)
endfunction()