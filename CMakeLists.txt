 
cmake_minimum_required(VERSION 3.10)
project(libh5pp  VERSION 1.0.0)


set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)


################################################################
### Set options for compilation                              ###
################################################################
option(BUILD_SHARED_LIBS  "Builds shared libraries"                             OFF)
option(BUILD_EXAMPLES     "Build examples"                                      OFF)
option(ENABLE_TESTS       "Enable testing"                                      ON)
option(DOWNLOAD_HDF5      "Download and install HDF5 if not found"              OFF)
option(DOWNLOAD_SPDLOG    "Download and install spdlog if not found"            OFF)
option(DOWNLOAD_EIGEN3    "Download and install Eigen3 if not found"            OFF)
option(DOWNLOAD_ALL       "Download and install all dependencies if not found"  OFF)


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

#cmake_policy(SET CMP0074 NEW)
#cmake_policy(SET CMP0075 NEW)


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



set(INSTALL_DIRECTORY_H5PP             ${CMAKE_INSTALL_PREFIX})
set(BUILD_DIRECTORY_H5PP               ${CMAKE_BINARY_DIR}/h5pp)
set(INSTALL_DIRECTORY_THIRD_PARTY      ${CMAKE_INSTALL_PREFIX}/third-party)
set(BUILD_DIRECTORY_THIRD_PARTY        ${CMAKE_BINARY_DIR}/third-party-build)
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


include(GNUInstallDirs)
set(INSTALL_CONFIGDIR share/cmake/h5pp)
set(H5PP_ROOT ${CMAKE_INSTALL_PREFIX})
set(H5PP_DIR  ${INSTALL_CONFIGDIR})


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
configure_package_config_file(
        cmake/h5ppConfig.cmake.in
        ${PROJECT_BINARY_DIR}/h5ppConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS
        H5PP_DIR
        Eigen3_DIR EIGEN3_INCLUDE_DIR EIGEN3_ROOT_DIR
        spdlog_DIR
        HDF5_DIR
        INSTALL_DIRECTORY_THIRD_PARTY
)



write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)
##Install the config, configversion and custom find modules
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
        )


##Register package in user's package registry
export(PACKAGE h5pp)








############################
# UNIT TEST
############################
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests/simpleWrite EXCLUDE_FROM_ALL)
    add_subdirectory(tests/readWrite EXCLUDE_FROM_ALL)

    add_custom_target(all_tests ALL
            DEPENDS simpleWrite
            DEPENDS readWrite
            DEPENDS h5pp
            )
    add_dependencies(${PROJECT_NAME} all_tests h5pp)
#    add_custom_command(TARGET ${PROJECT_NAME}
#            POST_BUILD
#            COMMENT "Running Tests"
#            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#            DEPENDS all_tests
#            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --output-on-failures)
    add_custom_command(TARGET all_tests
            POST_BUILD
            COMMENT "Running Tests"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PROJECT_NAME}
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --output-on-failures)

endif()




############################
# BUILD EXAMPLES
############################
if(BUILD_EXAMPLES)
    add_subdirectory(examples/00_helloworld)
    add_subdirectory(examples/01_writescalar)

    add_custom_target(all_examples ALL
            DEPENDS helloworld
            DEPENDS writescalar
            )
    add_dependencies(${PROJECT_NAME} all_examples)
endif()

