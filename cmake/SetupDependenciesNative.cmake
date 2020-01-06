

# This makes sure we use our modules to find dependencies!
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Fetch_spdlog.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Fetch_Eigen3.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Fetch_HDF5.cmake)

if(TARGET Eigen3::Eigen)
    target_link_libraries(deps INTERFACE Eigen3::Eigen)
    get_target_property(EIGEN3_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${EIGEN3_INCLUDE_DIR})
    list(APPEND TARGETS_FOUND Eigen3::Eigen)
else()
    list(APPEND TARGETS_NOT_FOUND Eigen3::Eigen)
endif()


if(TARGET spdlog::spdlog)
    target_link_libraries(deps INTERFACE spdlog::spdlog)
    get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${SPDLOG_INCLUDE_DIR})
    list(APPEND TARGETS_FOUND spdlog::spdlog)
else()
    list(APPEND TARGETS_NOT_FOUND spdlog::spdlog)
endif()


if(TARGET hdf5::hdf5)
    target_link_libraries(deps INTERFACE hdf5::hdf5)
    get_target_property(HDF5_INCLUDE_DIR hdf5::hdf5 INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${HDF5_INCLUDE_DIR})
    list(APPEND TARGETS_FOUND hdf5)
else()
    list(APPEND TARGETS_NOT_FOUND hdf5::hdf5)
endif()


foreach(tgt ${TARGETS_NOT_FOUND})
    message(STATUS "Dependency not found: [${tgt}] -- make sure to find and link to it manually before using h5pp")
endforeach()

if(TARGETS_NOT_FOUND)
    set(ALL_TARGETS_FOUND FALSE)
else()
    set(ALL_TARGETS_FOUND TRUE)
endif()


##################################################################
### Link all the things!                                       ###
##################################################################
target_link_libraries(deps INTERFACE Eigen3::Eigen)
target_link_libraries(deps INTERFACE spdlog::spdlog)
target_link_libraries(deps INTERFACE hdf5::hdf5)


