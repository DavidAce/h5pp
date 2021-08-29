
if(H5PP_PACKAGE_MANAGER MATCHES "find")
    if(H5PP_PACKAGE_MANAGER STREQUAL "find")
        set(REQUIRED REQUIRED)
    endif()
    # Start finding the dependencies
    if(H5PP_ENABLE_EIGEN3 AND NOT Eigen3_FOUND )
        find_package(Eigen3 3.3 ${REQUIRED})
        if(Eigen3_FOUND)
            target_link_libraries(deps INTERFACE Eigen3::Eigen)
        endif()
    endif()

    if(H5PP_ENABLE_FMT AND NOT fmt_FOUND)
        find_package(fmt 6.1.2 ${REQUIRED})
        if(fmt_FOUND)
            target_link_libraries(deps INTERFACE fmt::fmt)
        endif()
    endif()
    if(H5PP_ENABLE_SPDLOG AND spdlog_FOUND)
        find_package(spdlog 1.3.1 ${REQUIRED})
        if(spdlog_FOUND AND TARGET spdlog AND NOT TARGET spdlog::spdlog)
            add_library(spdlog::spdlog ALIAS spdlog)
        endif()
        if(spdlog_FOUND)
            target_link_libraries(deps INTERFACE spdlog::spdlog)
        endif()
    endif()
    if (NOT HDF5_FOUND)
        find_package(HDF5 1.8 COMPONENTS C HL ${REQUIRED})
        if(HDF5_FOUND)
            if(BUILD_SHARED_LIBS)
                set(HDF5_LINK_TYPE shared)
            else()
                set(HDF5_LINK_TYPE static)
            endif()
            list(APPEND HDF5_HL_TARGET_CANDIDATES
                    hdf5::hdf5_hl
                    hdf5::hdf5_hl-${HDF5_LINK_TYPE}
                    hdf5_hl
                    hdf5_hl-${HDF5_LINK_TYPE}
                    hdf5::hdf5_hl-static
                    hdf5_hl-static
                    hdf5::hdf5_hl-shared
                    hdf5_hl-shared
                    )
            mark_as_advanced(HDF5_HL_TARGET_CANDIDATES)
            foreach(tgt ${HDF5_HL_TARGET_CANDIDATES})
                if(TARGET ${tgt})
                    set(hdf5_TARGET ${tgt})
                    mark_as_advanced(hdf5_TARGET)
                    break()
                endif()
            endforeach()
            if(NOT hdf5_TARGET)
                message(FATAL_ERROR "HDF5 was found but the target name for HL component is not recognized")
            endif()
            target_link_libraries(deps INTERFACE ${hdf5_TARGET})
        endif()
    endif()

endif()