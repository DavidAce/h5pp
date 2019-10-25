 
cmake_minimum_required(VERSION 3.10)
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
add_library(h5pp::flags ALIAS flags)

target_compile_features(flags INTERFACE cxx_std_17)
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
add_library(h5pp::deps ALIAS deps)
if(TARGET Eigen3::Eigen)
    add_dependencies(deps Eigen3::Eigen)
    target_link_libraries(deps INTERFACE Eigen3::Eigen)
    get_target_property(EIGEN_INFO Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
#    install(TARGETS Eigen3 EXPORT h5ppTargets )
endif()
if(TARGET spdlog::spdlog)
    add_dependencies(deps spdlog::spdlog)
    target_link_libraries(deps INTERFACE spdlog::spdlog)
    get_target_property(SPDLOG_INFO spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
#    install(TARGETS spdlog EXPORT h5ppTargets )

endif()
if(TARGET hdf5::hdf5)
    add_dependencies(deps hdf5::hdf5)
    target_link_libraries(deps INTERFACE hdf5::hdf5)
    get_target_property(HDF5_INFO hdf5::hdf5 INTERFACE_LINK_LIBRARIES)
#    install(TARGETS hdf5 EXPORT h5ppTargets )
endif()
install(TARGETS h5pp deps flags EXPORT h5ppTargets )



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
        add_subdirectory(tests/largeWrite           EXCLUDE_FROM_ALL)
        add_subdirectory(tests/overWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWrite            EXCLUDE_FROM_ALL)
        add_subdirectory(tests/readWriteAttributes  EXCLUDE_FROM_ALL)
        add_subdirectory(tests/copySwap             EXCLUDE_FROM_ALL)

        add_custom_target(all_tests ALL
                DEPENDS simpleWrite
                DEPENDS largeWrite
                DEPENDS overWrite
                DEPENDS readWrite
                DEPENDS readWriteAttributes
                DEPENDS copySwap
                )

        add_custom_command(TARGET all_tests
                POST_BUILD
                COMMENT "Running Tests:"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS h5pp::deps all_tests
                COMMAND ${CMAKE_CTEST_COMMAND} --build-config $<CONFIG> --output-on-failures)
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
    include(cmake-modules/Build_examples.cmake)
    add_custom_target(examples
            DEPENDS example_helloworld
            DEPENDS example_writescalar
            DEPENDS ${PROJECT_NAME})
#    add_dependencies(examples ${PROJECT_NAME})

else()
    message(STATUS "Dependencies missing, ignoring BUILD_EXAMPLES=ON")
endif()
