 
cmake_minimum_required(VERSION 3.12)
project(h5pp  VERSION 0.2.2)


set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake-modules)

################################################################
### Get git version number                                   ###
### Generates a header gitversion/gitversion.h               ###
### Include it using #include <gitversion.h>                 ###
### Gives a namespace GIT:: with several git version numbers.###
################################################################
include(cmake-modules/gitversion.cmake)


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
message("=============================== DETECTED OS =================================")
message("${OS_PROPERTIES}")
message("=============================================================================")


################################################################
### Set options for compilation                              ###
################################################################
option(BUILD_SHARED_LIBS  "Builds shared libraries"                             OFF)
option(BUILD_EXAMPLES     "Builds examples"                                     OFF)
option(ENABLE_TESTS       "Enable testing"                                      ON)
option(DOWNLOAD_MISSING   "Download and install all dependencies if not found"  OFF)


message("=========================== h5pp build options ==============================")
message(STATUS "BUILD_SHARED_LIBS  : ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_EXAMPLES     : ${BUILD_EXAMPLES}")
message(STATUS "ENABLE_TESTS       : ${ENABLE_TESTS}")
message(STATUS "DOWNLOAD_MISSING   : ${DOWNLOAD_MISSING}")
message(STATUS "THIRD_PARTY_DIR    : ${THIRD_PARTY_DIR}")
message("=============================================================================")



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




#################################
### Compiler and linker flags ###
#################################
add_library(flags INTERFACE)
target_compile_options(flags INTERFACE  $<$<COMPILE_LANGUAGE:CXX>: -lstdc++fs -std=c++17>)
target_link_libraries (flags INTERFACE -lstdc++fs)
if(GCC_TOOLCHAIN)
    target_compile_options(flags INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
endif()


#######################################################
###  Check  #include<filesystem> or                 ###
###  or     #include<experimental/filesystem>       ###
#######################################################
include(cmake-modules/CheckCXX17FilesystemCompiles.cmake)
CheckCXX17FilesystemCompiles()

#######################################################
###  Check  #include<experimental/type_traits>      ###
#######################################################
include(cmake-modules/CheckTypeTraitsCompiles.cmake)
CheckTypeTraitsCompiles()




#########################################################################
### Define an interface library and establish its needs for the user  ###
#########################################################################
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${INSTALL_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG FALSE)
find_package(Threads REQUIRED)
if(NOT BUILD_SHARED_LIBS)
    set_target_properties(Threads::Threads PROPERTIES INTERFACE_LINK_LIBRARIES "-Wl,--whole-archive ${CMAKE_THREAD_LIBS_INIT} -Wl,--no-whole-archive")
endif()

##################################################################
### Adapt pthread for static/dynamic linking                   ###
##################################################################
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG FALSE)
find_package(Threads)
if(TARGET Threads::Threads AND NOT BUILD_SHARED_LIBS)
    set_target_properties(Threads::Threads PROPERTIES INTERFACE_LINK_LIBRARIES "-Wl,--whole-archive ${CMAKE_THREAD_LIBS_INIT} -Wl,--no-whole-archive")
endif()




##################################################################
### Setup dependency paths                                     ###
##################################################################
include(GNUInstallDirs)
set(H5PP_INSTALL_DIR                   ${CMAKE_INSTALL_PREFIX})
set(H5PP_BUILD_DIR                     ${CMAKE_BINARY_DIR}/h5pp)
if(THIRD_PARTY_DIR)
    set(H5PP_INSTALL_DIR_THIRD_PARTY       ${THIRD_PARTY_DIR})
else()
    set(H5PP_INSTALL_DIR_THIRD_PARTY       ${CMAKE_INSTALL_PREFIX}/third-party)
endif()
set(H5PP_BUILD_DIR_THIRD_PARTY         ${CMAKE_BINARY_DIR}/third-party-build)
set(H5PP_CONFIG_DIR_THIRD_PARTY        ${CMAKE_BINARY_DIR}/third-party-config)
set(DIRECTORY_HINTS
        ${THIRD_PARTY_DIR}
        ${H5PP_INSTALL_DIR_THIRD_PARTY}
        ${CMAKE_INSTALL_PREFIX}
        ${CMAKE_INSTALL_PREFIX}/third-party
        ${H5PP_INSTALL_DIR_THIRD_PARTY}
        ${CMAKE_SOURCE_DIR}
        $ENV{HOME}/.conda
        $ENV{HOME}/anaconda3
        $ENV{HOME}/miniconda

)


##################################################################
### Try to fetch all dependencies                              ###
##################################################################
include(cmake-modules/Fetch_Eigen3.cmake)
include(cmake-modules/Fetch_spdlog.cmake)
include(cmake-modules/Fetch_HDF5.cmake)

if(TARGET hdf5::hdf5 AND TARGET Eigen3::Eigen AND TARGET spdlog::spdlog)
    set(DEPENDENCIES_FOUND TRUE)
else()
    set(DEPENDENCIES_FOUND FALSE)
endif()


##################################################################
### Define interface libraries for the dependencies            ###
##################################################################
add_library(deps INTERFACE)
if(TARGET Eigen3::Eigen)
    target_link_libraries(deps INTERFACE Eigen3::Eigen)
endif()
if(TARGET spdlog::spdlog)
    target_link_libraries(deps INTERFACE spdlog::spdlog)
endif()
if(TARGET hdf5::hdf5)
    target_link_libraries(deps INTERFACE hdf5::hdf5)
endif()


##################################################################
### Print summary of CMake configuration                       ###
##################################################################
message("=============================================================================")
message("===                  SUMMARY OF CMAKE CONFIGURATION                       ===")
message("=============================================================================")
include(cmake-modules/PrintTargetInfo.cmake)
print_target_info(h5pp)
print_target_info(deps)
print_target_info(flags)
print_target_info(spdlog::spdlog)
print_target_info(Eigen3::Eigen)
print_target_info(hdf5::hdf5)
message("=============================================================================")
message("")



if(NOT DEPENDENCIES_FOUND)
message(WARNING "Dependencies missing.\nTo use h5pp headers, link the missing dependencies manually from your project")
endif()



#install(TARGETS test DESTINATION wildtest/myproj EXPORT myproj-targets)
#install(EXPORT myproj-targets DESTINATION lib/myproj)


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


install(TARGETS h5pp deps flags EXPORT h5ppTargets)

#Export the targets to a script
install(EXPORT
        h5ppTargets
        FILE h5ppTargets.cmake
        NAMESPACE h5pp::
        DESTINATION ${H5PP_INSTALL_CONFIGDIR})


#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)


configure_package_config_file(
        cmake-modules/h5ppConfig.cmake.in
        ${PROJECT_BINARY_DIR}/h5ppConfig.cmake
        INSTALL_DESTINATION ${H5PP_INSTALL_CONFIGDIR}
        PATH_VARS H5PP_ROOT H5PP_DIR
        H5PP_INSTALL_DIR_THIRD_PARTY
        H5PP_INSTALL_CONFIGDIR
        DIRECTORY_HINTS
)

configure_package_config_file(
        cmake-modules/h5ppConfig.deps.cmake.in
        ${PROJECT_BINARY_DIR}/h5ppConfig.deps.cmake
        INSTALL_DESTINATION ${H5PP_INSTALL_CONFIGDIR}
        PATH_VARS H5PP_ROOT H5PP_DIR
        H5PP_INSTALL_DIR_THIRD_PARTY
        H5PP_INSTALL_CONFIGDIR
        DIRECTORY_HINTS
)



write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)


##Install the config, configversion and custom find modules
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
        cmake-modules/FindEigen3.cmake
        cmake-modules/FindPackageHDF5.cmake
        DESTINATION ${H5PP_INSTALL_CONFIGDIR}
        )



##################################################################
### Unit testing                                               ###
##################################################################
include(tests/all-tests.cmake)


############################
# ADD TARGETS FOR  EXAMPLES
############################

if(DEPENDENCIES_FOUND)
    set(H5PP_INSTALL_DIR_EXAMPLES  ${CMAKE_INSTALL_PREFIX}/examples)
    set(H5PP_BUILD_DIR_EXAMPLES    ${CMAKE_BINARY_DIR}/examples)
    include(cmake-modules/Build_examples.cmake)
    add_custom_target(examples
            DEPENDS example_helloworld
            DEPENDS example_writescalar
            DEPENDS ${PROJECT_NAME})
#    add_dependencies(examples ${PROJECT_NAME})

else()
    message(STATUS "Dependencies missing, ignoring BUILD_EXAMPLES=ON")
endif()
