
if(H5PP_PACKAGE_MANAGER MATCHES "find")
    if(H5PP_PACKAGE_MANAGER STREQUAL "find")
        set(REQUIRED REQUIRED)
    endif()
    # Start finding the dependencies
    if(H5PP_ENABLE_EIGEN3 AND NOT TARGET Eigen3::Eigen )
        find_package(Eigen3 3.3 ${REQUIRED})
        if(Eigen3_FOUND AND TARGET Eigen3::Eigen)
            list(APPEND H5PP_TARGETS Eigen3::Eigen)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        endif()
    endif()

    if(H5PP_ENABLE_FMT AND NOT TARGET fmt::fmt)
        find_package(fmt 6.1.2 ${REQUIRED})
        if(fmt_FOUND AND TARGET fmt::fmt)
            list(APPEND H5PP_TARGETS fmt::fmt)
            target_link_libraries(deps INTERFACE fmt::fmt)
        endif()
    endif()
    if(H5PP_ENABLE_SPDLOG AND NOT TARGET spdlog::spdlog)
        find_package(spdlog 1.3.1 ${REQUIRED})
        if(spdlog_FOUND AND TARGET spdlog AND NOT TARGET spdlog::spdlog)
            add_library(spdlog::spdlog ALIAS spdlog)
        endif()
        if(spdlog_FOUND AND TARGET spdlog::spdlog)
            list(APPEND H5PP_TARGETS spdlog::spdlog)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        endif()
    endif()
    if (NOT TARGET hdf5::all)
        find_package(HDF5 1.8 COMPONENTS C HL ${REQUIRED})
        if(HDF5_FOUND AND TARGET hdf5::all)
            list(APPEND H5PP_TARGETS hdf5::all)
            target_link_libraries(deps INTERFACE hdf5::all)
        endif()
    endif()
endif()