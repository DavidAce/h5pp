cmake_minimum_required(VERSION 3.10)
project(helloworld)
add_executable(${PROJECT_NAME} helloworld.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE -Wpedantic -Wextra -O0)
find_package(h5pp PATHS ${CMAKE_INSTALL_PREFIX} PATH_SUFFIXES h5pp REQUIRED)


if (h5pp_FOUND)
    get_target_property(EIGEN_INFO   Eigen3::Eigen      INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(SPDLOG_INFO  spdlog::spdlog     INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(HDF5_INFO    hdf5::hdf5         INTERFACE_LINK_LIBRARIES)
    message("=============================================================================")
    message("===            SUMMARY OF DEPENDENCIES FOUND BY HELLOWORLD                ===")
    message("=============================================================================")
    message("    EIGEN3   :   ${EIGEN_INFO}")
    message("    SPDLOG   :   ${SPDLOG_INFO}")
    message("    HDF5     :   ${HDF5_INFO}")
    message("=============================================================================")
    message("")

    target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp h5pp::deps h5pp::flags )
endif()


