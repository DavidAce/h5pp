 
cmake_minimum_required(VERSION 3.10)
project(libh5pp  VERSION 1.0.0)


set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)


################################################################
### Set options for compilation                              ###
################################################################
option(BUILD_SHARED_LIBS  "Builds shared libraries (libh5pp.so)"                OFF)
option(BUILD_EXAMPLES     "Build examples"                                      ON)
option(DOWNLOAD_HDF5      "Download and install HDF5 if not found"              ON)
option(DOWNLOAD_SPDLOG    "Download and install spdlog if not found"            ON)
option(DOWNLOAD_EIGEN3    "Download and install Eigen3 if not found"            ON)
option(DOWNLOAD_ALL       "Download and install all dependencies if not found"  ON)


#################################################################
### Set default build type for compilation if none was given  ###
#################################################################
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to Release as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release")
endif ()



if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif()

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0075 NEW)


##################################################################
### Get operating system properties into a variable            ###
##################################################################
set(GET_OS_INFO_CMD lsb_release -a)
if(${CMAKE_HOST_APPLE})
    set(GET_OS_INFO_CMD "sw_vers")
endif()
execute_process(COMMAND ${GET_OS_INFO_CMD}
        OUTPUT_VARIABLE OS_PROPERTIES
        OUTPUT_STRIP_TRAILING_WHITESPACE)
message("========= DETECTED OS =========")
message("${OS_PROPERTIES}")
message("===============================")



set(INSTALL_DIRECTORY             ${CMAKE_INSTALL_PREFIX})
set(BUILD_DIRECTORY               ${CMAKE_BINARY_DIR})
set(INSTALL_DIRECTORY_THIRD_PARTY ${CMAKE_INSTALL_PREFIX}/third-party)
set(BUILD_DIRECTORY_THIRD_PARTY   ${CMAKE_BINARY_DIR}/third-party-build)
include(cmake/Fetch_Eigen3.cmake)
include(cmake/Fetch_spdlog.cmake)
include(cmake/Fetch_HDF5.cmake)
include(cmake/Fetch_h5pp.cmake)




get_target_property(EIGEN_INFO Eigen3 INTERFACE_LINK_LIBRARIES)
get_target_property(SPDLOG_INFO spdlog INTERFACE_LINK_LIBRARIES)
get_target_property(HDF5_INFO hdf5 INTERFACE_LINK_LIBRARIES)

message("=============================================================================")
message("===            SUMMARY OF CMAKE CONFIGURATION FOR libh5pp                 ===")
message("=============================================================================")
message("    EIGEN3   :   ${EIGEN_INFO}")
message("    SPDLOG   :   ${SPDLOG_INFO}")
message("    HDF5     :   ${HDF5_INFO}")
message("=============================================================================")
message("")


# Bind projects together
add_custom_target(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} h5pp hdf5 Eigen3 spdlog)
target_link_libraries(h5pp INTERFACE hdf5 Eigen3 spdlog)

#######################################################
### Install library                                 ###
### Default path is ${PROJECT_SOURCE_DIR}/install   ###
### where ${PROJECT_SOURCE_DIR}  is the path of     ###
### this CMakeLists.txt                             ###
#######################################################




set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_PREFIX}/h5pp/lib/cmake/h5pp)

install(TARGETS h5pp Eigen3 hdf5 spdlog
        EXPORT        h5ppTargets
        )

#Export the targets to a script
install(EXPORT
        h5ppTargets
        FILE h5ppTargets.cmake
        NAMESPACE h5pp::
        EXPORT_LINK_INTERFACE_LIBRARIES
        DESTINATION ${INSTALL_CONFIGDIR})

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)


configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/h5ppConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS
        Eigen3_DIR EIGEN3_INCLUDE_DIR EIGEN3_ROOT_DIR
        spdlog_DIR
        HDF5_DIR
        INSTALL_DIRECTORY_THIRD_PARTY
)

##Install the config, configversion and custom find modules
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
        )


##Register package in user's package registry
export(PACKAGE h5pp)
