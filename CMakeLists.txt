 
cmake_minimum_required(VERSION 3.10)
project(h5pp  VERSION 0.4.4)


set(CMAKE_VERBOSE_MAKEFILE OFF)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)
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
option(BUILD_SHARED_LIBS        "Builds shared libraries"                             OFF)
option(BUILD_EXAMPLES           "Builds examples"                                     OFF)
option(ENABLE_TESTS             "Enable testing"                                      OFF)
option(DOWNLOAD_MISSING         "Download and install all dependencies if not found"  OFF)
option(H5PP_IS_SUBPROJECT       "Use h5pp with add_subdirectory()"                    OFF)
option(APPEND_LIBSUFFIX         "Append <libname> to CMAKE_INSTALL_PREFIX"            OFF)
option(PREFER_CONDA_LIBS        "Search for dependencies from anaconda first"         OFF)


# Used when h5pp is included as subproject (e.g., as Git submodule/subtree) in the source
# tree of a project that uses it. Users may set the non-cached variable H5PP_IS_SUBPROJECT
# before add_subdirectory(h5pp)
if(NOT H5PP_IS_SUBPROJECT)
    if("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
        set (H5PP_IS_SUBPROJECT FALSE)
    else()
        message(STATUS "Detected usage as subproject")
        set (H5PP_IS_SUBPROJECT TRUE)
    endif()
endif()


# Give default install directory for h5pp and its dependencies.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT H5PP_IS_SUBPROJECT)
    message(STATUS "Setting default install prefix -- CMAKE_INSTALL_PREFIX --> ${CMAKE_BINARY_DIR}/install")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "..." FORCE)
endif()


message("=========================== h5pp build options ==============================")
message(STATUS "BUILD_SHARED_LIBS       : ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_EXAMPLES          : ${BUILD_EXAMPLES}")
message(STATUS "ENABLE_TESTS            : ${ENABLE_TESTS}")
message(STATUS "DOWNLOAD_MISSING        : ${DOWNLOAD_MISSING}")
message(STATUS "CMAKE_INSTALL_PREFIX    : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "APPEND_LIBSUFFIX        : ${APPEND_LIBSUFFIX}")
message(STATUS "PREFER_CONDA_LIBS       : ${PREFER_CONDA_LIBS}")
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
target_compile_features(flags INTERFACE cxx_std_17)
target_link_libraries (flags INTERFACE -lstdc++fs)
if(GCC_TOOLCHAIN)
    target_compile_options(flags INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
endif()


#######################################################
###  Check  #include<filesystem> or                 ###
###  or     #include<experimental/filesystem>       ###
#######################################################
include(${PROJECT_SOURCE_DIR}/cmake-modules/CheckCXXFilesystem.cmake)
CheckCXXFilesystem()

#######################################################
###  Check  #include<experimental/type_traits>      ###
#######################################################
include(${PROJECT_SOURCE_DIR}/cmake-modules/CheckTypeTraits.cmake)
CheckTypeTraits()

#######################################################
###  Check  #include<optional> or                   ###
###  or     #include<experimental/optional>         ###
#######################################################
include(${PROJECT_SOURCE_DIR}/cmake-modules/CheckCXXOptional.cmake)
CheckCXXOptional()



#########################################################################
### Define an interface library and establish its needs for the user  ###
#########################################################################
add_library(${PROJECT_NAME} INTERFACE)


if(APPEND_LIBSUFFIX)
    set(h5pp_destination    ${CMAKE_INSTALL_PREFIX}/h5pp)
else()
    set(h5pp_destination    ${CMAKE_INSTALL_PREFIX})
endif()
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/h5pp/source/include>
        $<INSTALL_INTERFACE:${h5pp_destination}/include>
)


##################################################################
### Setup dependency paths                                     ###
### Prepend conda dirs for hints when using find_package       ###
##################################################################
include(GNUInstallDirs)
if(PREFER_CONDA_LIBS)
    list(APPEND H5PP_DIRECTORY_HINTS
            $ENV{CONDA_PREFIX}
            $ENV{HOME}/anaconda3
            $ENV{HOME}/miniconda
            $ENV{HOME}/.conda
            )
endif()
list(APPEND H5PP_DIRECTORY_HINTS ${CMAKE_INSTALL_PREFIX})





##################################################################
### Try to fetch all dependencies                              ###
##################################################################
include(${PROJECT_SOURCE_DIR}/cmake-modules/Fetch_Eigen3.cmake)
include(${PROJECT_SOURCE_DIR}/cmake-modules/Fetch_spdlog.cmake)
include(${PROJECT_SOURCE_DIR}/cmake-modules/Fetch_HDF5.cmake)


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
    get_target_property(EIGEN3_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${EIGEN3_INCLUDE_DIR})
endif()
if(TARGET spdlog::spdlog)
    target_link_libraries(deps INTERFACE spdlog::spdlog)
    get_target_property(SPDLOG_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${SPDLOG_INCLUDE_DIR})
endif()
if(TARGET hdf5::hdf5)
    target_link_libraries(deps INTERFACE hdf5::hdf5)
    get_target_property(HDF5_ROOT_DIR Eigen3::Eigen INTERFACE_LINK_LIBRARIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${HDF5_ROOT_DIR}/..)
endif()


##################################################################
### Print summary of CMake configuration                       ###
##################################################################
message("=============================================================================")
message("===                H5PP: SUMMARY OF CMAKE CONFIGURATION                   ===")
message("=============================================================================")
include(${PROJECT_SOURCE_DIR}/cmake-modules/PrintTargetInfo.cmake)
print_target_info(h5pp)
print_target_info(deps)
print_target_info(flags)
print_target_info(spdlog::spdlog)
print_target_info(Eigen3::Eigen)
print_target_info(hdf5::hdf5)
print_target_info(Threads::Threads)

message("=============================================================================")
message("")



if(NOT DEPENDENCIES_FOUND)
    message(WARNING "Dependencies missing.\nTo use h5pp headers, link the missing dependencies manually from your project")
endif()



if(H5PP_IS_SUBPROJECT)
    add_library(h5pp::h5pp ALIAS h5pp)
    add_library(h5pp::deps ALIAS deps)
    add_library(h5pp::flags ALIAS h5pp)

else()

    #######################################################
    ### Install library                                 ###
    ### Default path is ${PROJECT_SOURCE_DIR}/install   ###
    ### where ${PROJECT_SOURCE_DIR}  is the path of     ###
    ### this CMakeLists.txt                             ###
    #######################################################

    # Append directory to CMAKE_INSTALL_PREFIX
    # Useful if you want to have separate directories for each libs
    # and to easily delete them individually
    if(APPEND_LIBSUFFIX)
        set(h5pp_destination    ${CMAKE_INSTALL_PREFIX}/h5pp)
        set(eigen3_destination  ${CMAKE_INSTALL_PREFIX}/Eigen3)
        set(spdlog_destination  ${CMAKE_INSTALL_PREFIX}/spdlog)
        set(hdf5_destination    ${CMAKE_INSTALL_PREFIX}/hdf5)
    else()
        set(h5pp_destination    ${CMAKE_INSTALL_PREFIX})
        set(eigen3_destination  ${CMAKE_INSTALL_PREFIX})
        set(spdlog_destination  ${CMAKE_INSTALL_PREFIX})
        set(hdf5_destination    ${CMAKE_INSTALL_PREFIX})
    endif()


    include(GNUInstallDirs)
    # Read about this share path here https://cmake.org/cmake/help/v3.12/command/find_package.html
    set(H5PP_CONFIG_DIR ${h5pp_destination}/share/h5pp/cmake)
    set(H5PP_DIR  ${H5PP_CONFIG_DIR})
    set(H5PP_ROOT ${h5pp_destination})
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/h5pp/source/include/ DESTINATION ${h5pp_destination}/${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)
    if(EXISTS ${CMAKE_BINARY_DIR}/h5pp-deps-install/Eigen3)
        install(DIRECTORY ${CMAKE_BINARY_DIR}/h5pp-deps-install/Eigen3/ DESTINATION ${eigen3_destination} COMPONENT eigen3-dep)
    endif()
    if(EXISTS ${CMAKE_BINARY_DIR}/h5pp-deps-install/spdlog)
        install(DIRECTORY ${CMAKE_BINARY_DIR}/h5pp-deps-install/spdlog/ DESTINATION ${spdlog_destination} COMPONENT spdlog-dep)
    endif()
    if(EXISTS ${CMAKE_BINARY_DIR}/h5pp-deps-install/hdf5)
        install(DIRECTORY ${CMAKE_BINARY_DIR}/h5pp-deps-install/hdf5/ DESTINATION ${hdf5_destination}  COMPONENT hdf5-dep)
    endif()

    install(TARGETS h5pp flags EXPORT h5ppTargets)

    #Export the targets to a script
    install(EXPORT
            h5ppTargets
            FILE h5ppTargets.cmake
            NAMESPACE h5pp::
            DESTINATION ${H5PP_CONFIG_DIR})


    #Create a ConfigVersion.cmake file
    include(CMakePackageConfigHelpers)


    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake-modules/h5ppConfig.cmake.in
            ${PROJECT_BINARY_DIR}/h5ppConfig.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_ROOT H5PP_DIR H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake-modules/h5ppConfig.deps.cmake.in
            ${PROJECT_BINARY_DIR}/h5ppConfig.deps.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_ROOT H5PP_DIR H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
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
            ${PROJECT_SOURCE_DIR}/cmake-modules/FindEigen3.cmake
            ${PROJECT_SOURCE_DIR}/cmake-modules/FindPackageHDF5.cmake
            DESTINATION ${H5PP_CONFIG_DIR}
            COMPONENT config
            )

endif()

##################################################################
### Unit testing                                               ###
##################################################################
if(NOT H5PP_IS_SUBPROJECT AND ENABLE_TESTS)
    if(DEPENDENCIES_FOUND)
        enable_testing()
        add_subdirectory(tests/simpleWrite          )
        add_subdirectory(tests/largeWrite           )
        add_subdirectory(tests/overWrite            )
        add_subdirectory(tests/readWrite            )
        add_subdirectory(tests/readWriteAttributes  )
        add_subdirectory(tests/copySwap             )
        add_custom_target(all-tests
                DEPENDS test-simpleWrite
                        test-largeWrite
                        test-overWrite
                        test-readWrite
                        test-readWriteAttributes
                        test-copySwap)
        add_custom_command(
            TARGET all-tests
            POST_BUILD
            COMMENT "Running Tests"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS h5pp deps flags
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --output-on-failures)

    else()
        message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
    endif()
endif()




#############################
# ADD TARGETS FOR  EXAMPLES #
#############################
if(NOT H5PP_IS_SUBPROJECT AND BUILD_EXAMPLES)
    if(DEPENDENCIES_FOUND)
        set(H5PP_EXAMPLES_BINARY_DIR   ${CMAKE_BINARY_DIR}/h5pp-examples)
        add_subdirectory(h5pp/examples/helloworld  ${CMAKE_BINARY_DIR}/h5pp-examples/helloworld)
        add_subdirectory(h5pp/examples/writescalar ${CMAKE_BINARY_DIR}/h5pp-examples/writescalar)
        install(PROGRAMS ${CMAKE_BINARY_DIR}/h5pp-examples/helloworld/helloworld ${CMAKE_BINARY_DIR}/h5pp-examples/writescalar/writescalar DESTINATION ${h5pp_destination}/examples COMPONENT examples)
    else()
        message(STATUS "Dependencies missing, ignoring BUILD_EXAMPLES=ON")
    endif()
endif()



############################################
# Use CPACK to generate .deb install file  #
############################################
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL headers config)
# Define apt dependencies that work with this library
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libhdf5-dev (>=1.10), libeigen3-dev (>=3.3.4), libspdlog-dev (>=1.3)")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A C++ wrapper for HDF5")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/DavidAce/h5pp")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DavidAce <aceituno@kth.se>")
set(CPACK_DEBIAN_PACKAGE_NAME "h5pp")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT" )
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_GENERATOR "DEB")
include(CPack)