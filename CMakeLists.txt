 
cmake_minimum_required(VERSION 3.10)
project(h5pp  VERSION 0.2.2)


set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

################################################################
### Get git version number                                   ###
### Generates a header gitversion/gitversion.h               ###
### Include it using #include <gitversion.h>                 ###
### Gives a namespace GIT:: with several git version numbers.###
################################################################
include(cmake/gitversion.cmake)

################################################################
### Set options for compilation                              ###
################################################################
option(BUILD_SHARED_LIBS  "Builds shared libraries"                             OFF)
option(ENABLE_TESTS       "Enable testing"                                      ON)
option(DOWNLOAD_HDF5      "Download and install HDF5 if not found"              OFF)
option(DOWNLOAD_SPDLOG    "Download and install spdlog if not found"            OFF)
option(DOWNLOAD_EIGEN3    "Download and install Eigen3 if not found"            OFF)
option(DOWNLOAD_ALL       "Download and install all dependencies if not found"  OFF)


message("######## h5pp build options #########")
message(STATUS "BUILD_SHARED_LIBS  : ${BUILD_SHARED_LIBS}")
message(STATUS "ENABLE_TESTS       : ${ENABLE_TESTS}")
message(STATUS "DOWNLOAD_HDF5      : ${DOWNLOAD_HDF5}")
message(STATUS "DOWNLOAD_SPDLOG    : ${DOWNLOAD_SPDLOG}")
message(STATUS "DOWNLOAD_EIGEN3    : ${DOWNLOAD_EIGEN3}")
message(STATUS "DOWNLOAD_ALL       : ${DOWNLOAD_ALL}")
message("#####################################")



#################################################################
### Set default build type for compilation if none was given  ###
#################################################################
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to Release as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif ()



if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif()


#################################################################
### Set default policies if CMake is new enough               ###
#################################################################
if (CMAKE_VERSION VERSION_LESS 3.12)
    message(STATUS "Not setting policies")
else()
    cmake_policy(SET CMP0074 NEW)
    cmake_policy(SET CMP0075 NEW)
endif()



##################################################################
### Print operating system                                     ###
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




#################################
### Compiler and linker flags ###
#################################
add_library(flags INTERFACE)
add_library(h5pp::flags ALIAS flags)

target_compile_features(flags INTERFACE cxx_std_17)
if (NOT BUILD_SHARED_LIBS)
    target_link_libraries (flags INTERFACE -static)
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
    target_compile_options(flags INTERFACE -std=c++17)
    target_link_libraries (flags INTERFACE -lstdc++fs)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(flags INTERFACE -stdlib=libstdc++ -std=c++17)
    target_link_libraries (flags INTERFACE -stdlib=libstdc++ -lstdc++fs)
    if(GCC_TOOLCHAIN)
        target_compile_options(flags INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
    endif()
endif()


#set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
#set(THREADS_PREFER_PTHREAD_FLAG TRUE)
#find_package(Threads)
#set(PTHREAD_LIBRARY "-Wl,--whole-archive ${CMAKE_THREAD_LIBS_INIT} -Wl,--no-whole-archive")
#target_link_libraries (flags INTERFACE ${PTHREAD_LIBRARY})
target_compile_options(flags INTERFACE -pthread)
#########################################################################
### Define an interface library and establish its needs for the user  ###
#########################################################################
add_library(h5pp INTERFACE)
add_library(h5pp::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(
        h5pp
        INTERFACE
        $<BUILD_INTERFACE:${INSTALL_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
if (NOT BUILD_SHARED_LIBS)
    target_link_libraries (h5pp INTERFACE -static)
endif()


##################################################################
### Try to fetch all dependencies                              ###
##################################################################

set(H5PP_INSTALL_DIR                   ${CMAKE_INSTALL_PREFIX})
set(H5PP_BUILD_DIR                     ${CMAKE_BINARY_DIR}/h5pp)
set(H5PP_INSTALL_DIR_THIRD_PARTY       ${CMAKE_INSTALL_PREFIX}/third-party)
set(H5PP_BUILD_DIR_THIRD_PARTY         ${CMAKE_BINARY_DIR}/third-party-build)
include(cmake/Fetch_Eigen3.cmake)
include(cmake/Fetch_spdlog.cmake)
include(cmake/Fetch_HDF5.cmake)
if(TARGET hdf5::hdf5 AND TARGET Eigen3 AND TARGET spdlog)
    set(DEPENDENCIES_FOUND TRUE)
else()
    set(DEPENDENCIES_FOUND FALSE)
endif()

##################################################################
### Define interface libraries for the dependencies            ###
##################################################################


add_library(deps INTERFACE)
add_library(h5pp::deps ALIAS deps)
install(TARGETS h5pp deps flags EXPORT h5ppTargets )
if(TARGET Eigen3)
    add_dependencies(deps Eigen3)
    target_link_libraries(deps INTERFACE Eigen3)
    get_target_property(EIGEN_INFO Eigen3 INTERFACE_INCLUDE_DIRECTORIES)
    install(TARGETS Eigen3 EXPORT h5ppTargets )
endif()
if(TARGET spdlog)
    add_dependencies(deps spdlog)
    target_link_libraries(deps INTERFACE spdlog)
    get_target_property(SPDLOG_INFO spdlog INTERFACE_INCLUDE_DIRECTORIES)
    install(TARGETS spdlog EXPORT h5ppTargets )

endif()
if(TARGET hdf5::hdf5)
    add_dependencies(deps hdf5::hdf5)
    target_link_libraries(deps INTERFACE hdf5::hdf5)
    get_target_property(HDF5_INFO hdf5::hdf5 INTERFACE_LINK_LIBRARIES)
    install(TARGETS hdf5 EXPORT h5ppTargets )
endif()



message("=============================================================================")
message("===            SUMMARY OF DEPENDENCIES FOUND BY H5PP                      ===")
message("=============================================================================")
message("    EIGEN3   :   ${EIGEN_INFO}")
message("    SPDLOG   :   ${SPDLOG_INFO}")
message("    HDF5     :   ${HDF5_INFO}")
message("=============================================================================")
message("")

if(NOT DEPENDENCIES_FOUND)
message(WARNING "Dependencies missing.\nTo use h5pp headers, link the missing dependencies manually from your project")
endif()




    # Bind projects together
#if (DEPENDENCIES_FOUND)
#    add_custom_target(h5pp-builder ALL)
#    add_library(deps INTERFACE)
#    add_library(h5pp::deps ALIAS deps)
#    add_dependencies(deps hdf5::hdf5 Eigen3::Eigen spdlog::spdlog)
#    target_link_libraries(deps INTERFACE hdf5::hdf5 Eigen3 spdlog)

#    add_dependencies(deps hdf5::hdf5 Eigen3::Eigen spdlog::spdlog)
#    target_link_libraries(deps INTERFACE hdf5::hdf5 Eigen3::Eigen spdlog::spdlog)


#    target_link_libraries(${PROJECT_NAME} INTERFACE h5pp::deps)
#    add_dependencies(h5pp-builder h5pp::h5pp h5pp::deps)
#endif()








#######################################################
### Install library                                 ###
### Default path is ${PROJECT_SOURCE_DIR}/install   ###
### where ${PROJECT_SOURCE_DIR}  is the path of     ###
### this CMakeLists.txt                             ###
#######################################################


include(GNUInstallDirs)
set(H5PP_INSTALL_CONFIGDIR share/cmake/h5pp)
set(H5PP_ROOT ${CMAKE_INSTALL_PREFIX})
set(H5PP_DIR  ${H5PP_INSTALL_CONFIGDIR})
install(DIRECTORY h5pp/source/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})



#Export the targets to a script
install(EXPORT
        h5ppTargets
        FILE h5ppTargets.cmake
        NAMESPACE h5pp::
        DESTINATION ${H5PP_INSTALL_CONFIGDIR})

#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)

configure_package_config_file(
        cmake/h5ppConfig.cmake.in
        ${PROJECT_BINARY_DIR}/h5ppConfig.cmake
        INSTALL_DESTINATION ${H5PP_INSTALL_CONFIGDIR}
        PATH_VARS H5PP_DIR H5PP_ROOT
        H5PP_INSTALL_DIR_THIRD_PARTY
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
        DESTINATION ${H5PP_INSTALL_CONFIGDIR}
        )


###Register package in user's package registry
##export(PACKAGE h5pp)
#
#
#
#
#
#
#
#
#############################
## UNIT TEST
#############################
if(ENABLE_TESTS)
    if(DEPENDENCIES_FOUND)
        enable_testing()
        add_subdirectory(tests/simpleWrite          EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWriteAttributes  EXCLUDE_FROM_ALL)
        add_subdirectory(tests/copySwap             EXCLUDE_FROM_ALL)

        add_custom_target(all_tests ALL
                DEPENDS simpleWrite
                DEPENDS readWrite
                DEPENDS readWriteAttributes
                DEPENDS copySwap
                )

#        add_custom_command(TARGET all_tests
#                POST_BUILD
#                COMMENT "Running Tests:"
#                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#                DEPENDS h5pp::deps all_tests
#                COMMAND ${CMAKE_CTEST_COMMAND} --build-config $<CONFIG> --output-on-failures)
    else()
        message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
    endif()
endif()

#
#

############################
# ADD TARGETS FOR  EXAMPLES
############################

if(DEPENDENCIES_FOUND)
    set(H5PP_INSTALL_DIR_EXAMPLES  ${CMAKE_INSTALL_PREFIX}/examples)
    set(H5PP_BUILD_DIR_EXAMPLES    ${CMAKE_BINARY_DIR}/examples)
    include(cmake/Build_examples.cmake)
    add_custom_target(examples
            DEPENDS example_helloworld
            DEPENDS example_writescalar
            DEPENDS ${PROJECT_NAME})
#    add_dependencies(examples ${PROJECT_NAME})

else()
    message(STATUS "Dependencies missing, ignoring BUILD_EXAMPLES=ON")
endif()
