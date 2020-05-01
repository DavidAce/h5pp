if(H5PP_DOWNLOAD_METHOD MATCHES "fetch")
    # This makes sure we use our modules to find dependencies!
    list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)



    # Download spdlog
    if (H5PP_ENABLE_SPDLOG AND NOT TARGET spdlog::spdlog)
        find_package(spdlog 1.3
                PATH_SUFFIXES spdlog spdlog/share spdlog/cmake
                NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY )
        if(NOT TARGET spdlog::spdlog)
            message(STATUS "Spdlog will be installed into ${CMAKE_INSTALL_PREFIX}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(spdlog  "${CMAKE_INSTALL_PREFIX}" "")
            find_package(spdlog 1.3
                    PATH_SUFFIXES spdlog spdlog/share spdlog/cmake
                    NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY )
            if(TARGET spdlog::spdlog)
                message(STATUS "spdlog installed successfully")
            endif()
        endif()
        if(TARGET spdlog::spdlog)
            list(APPEND H5PP_TARGETS spdlog::spdlog)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        else()
            message(WARNING "Spdlog could not be downloaded and built from source")
        endif()
    endif()

    # Download Eigen3
    if (H5PP_ENABLE_EIGEN3 AND NOT TARGET Eigen3::Eigen)
        find_package(Eigen3 3.3.7
                PATH_SUFFIXES Eigen3 eigen3 include/Eigen3 include/eigen3
                NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY)
        if(NOT TARGET Eigen3::Eigen)
            message(STATUS "Eigen3 will be installed into ${CMAKE_INSTALL_PREFIX}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            build_dependency(Eigen3 "${CMAKE_INSTALL_PREFIX}" "")
            find_package(Eigen3 3.3.7
                    PATH_SUFFIXES Eigen3 eigen3 include/Eigen3 include/eigen3
                    NO_DEFAULT_PATH NO_CMAKE_PACKAGE_REGISTRY)
            if(TARGET Eigen3::Eigen)
                message(STATUS "Eigen3 installed successfully")
            endif()
        endif()
        if(TARGET Eigen3::Eigen)
            list(APPEND H5PP_TARGETS Eigen3::Eigen)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        else()
            message(WARNING "Eigen3 could not be downloaded and built from source")
        endif()
    endif()


    # Download HDF5
    if(NOT TARGET hdf5::hdf5)
        find_package(HDF5 1.8 COMPONENTS C HL)
        if(NOT TARGET hdf5::hdf5)
            message(STATUS "HDF5 will be installed into ${CMAKE_INSTALL_PREFIX}")
            include(${PROJECT_SOURCE_DIR}/cmake/BuildDependency.cmake)
            list(APPEND H5PP_HDF5_OPTIONS  "-DHDF5_ENABLE_PARALLEL:BOOL=${H5PP_ENABLE_MPI}")
            build_dependency(hdf5 "${CMAKE_INSTALL_PREFIX}" "${H5PP_HDF5_OPTIONS}")
            set(HDF5_ROOT ${CMAKE_INSTALL_PREFIX})
            set(HDF5_NO_DEFAULT_PATH ON)
            find_package(HDF5 1.8 COMPONENTS C HL REQUIRED)
            if(TARGET hdf5::hdf5)
                message(STATUS "hdf5 installed successfully")
            endif()
        endif()
        if(TARGET hdf5::hdf5)
            list(APPEND H5PP_TARGETS hdf5::hdf5)
            target_link_libraries(deps INTERFACE hdf5::hdf5)
        else()
            message(WARNING "HDF5 could not be downloaded and built from source")
        endif()
    endif()
endif()
