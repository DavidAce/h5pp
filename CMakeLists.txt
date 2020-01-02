 
cmake_minimum_required(VERSION 3.10)
project(h5pp  VERSION 1.0.0
        DESCRIPTION "A C++17 wrapper for HDF5"
        HOMEPAGE_URL "https://github.com/DavidAce/h5pp"
        )

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
option(BUILD_SHARED_LIBS        "Builds shared libraries"                             OFF)
option(BUILD_EXAMPLES           "Builds examples"                                     OFF)
option(ENABLE_TESTS             "Enable testing"                                      OFF)
option(H5PP_IS_SUBPROJECT       "Use h5pp with add_subdirectory()"                    OFF)
option(APPEND_LIBSUFFIX         "Append <libname> to CMAKE_INSTALL_PREFIX"            OFF)
option(PREFER_CONDA_LIBS        "Search for dependencies from anaconda first"         OFF)
option(H5PP_PRINT_INFO          "Print info during cmake configuration"               OFF)

#################################################################
### Make an "enum" for valid download methods:                ###
###     native                                                ###
###     conan                                                 ###
###     none                                                  ###
#################################################################
set(VALID_DOWNLOAD_METHODS conan native none)
set(DOWNLOAD_METHOD native CACHE STRING "Download method for external dependencies")
set_property(CACHE DOWNLOAD_METHOD  PROPERTY STRINGS ${VALID_DOWNLOAD_METHODS})
if(NOT DOWNLOAD_METHOD IN_LIST VALID_DOWNLOAD_METHODS)
    message(FATAL_ERROR "DOWNLOAD_METHOD must be one of ${VALID_DOWNLOAD_METHODS}")
endif()



##################################################################
### Print host properties                                      ###
##################################################################
if(H5PP_PRINT_INFO)
    cmake_host_system_information(RESULT _host_name   QUERY HOSTNAME)
    cmake_host_system_information(RESULT _proc_type   QUERY PROCESSOR_DESCRIPTION)
    cmake_host_system_information(RESULT _os_name     QUERY OS_NAME)
    cmake_host_system_information(RESULT _os_release  QUERY OS_RELEASE)
    cmake_host_system_information(RESULT _os_version  QUERY OS_VERSION)
    cmake_host_system_information(RESULT _os_platform QUERY OS_PLATFORM)
    message("========= HOST INFO =========")
    message("${_host_name}")
    message("${_os_name} ${_os_platform} ${_os_release}")
    message("${_proc_type}")
    message("${_os_version}")
    message("===============================")
endif()


#####################################################################
# Used when h5pp is included as subproject (e.g., as Git submodule/subtree) in the source
# tree of a project that uses it. Users may set the option H5PP_IS_SUBPROJECT
# before add_subdirectory(h5pp)
#####################################################################
if(NOT H5PP_IS_SUBPROJECT)
    if("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
        set (H5PP_IS_SUBPROJECT OFF)
    else()
        message(STATUS "Detected usage as subproject")
        set (H5PP_IS_SUBPROJECT ON)
    endif()
endif()


#####################################################################
### Set default install directory for h5pp and its dependencies   ###
### Append directory <libname> to CMAKE_INSTALL_PREFIX            ###
### Useful if you want to have separate directories for each libs ###
### and to easily delete them individually                        ###
#####################################################################
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT H5PP_IS_SUBPROJECT)
    message(STATUS "Setting default install prefix -- CMAKE_INSTALL_PREFIX --> ${CMAKE_BINARY_DIR}/install")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "..." FORCE)
endif()
if(APPEND_LIBSUFFIX)
    set(h5pp_relative_prefix    "h5pp/")
    set(h5pp_install_prefix    ${CMAKE_INSTALL_PREFIX}/h5pp)
    set(eigen3_install_prefix  ${CMAKE_INSTALL_PREFIX}/Eigen3)
    set(spdlog_install_prefix  ${CMAKE_INSTALL_PREFIX}/spdlog)
    set(hdf5_install_prefix    ${CMAKE_INSTALL_PREFIX}/hdf5)
else()
    set(h5pp_relative_prefix    "h5pp")
    set(h5pp_install_prefix    ${CMAKE_INSTALL_PREFIX})
    set(eigen3_install_prefix  ${CMAKE_INSTALL_PREFIX})
    set(spdlog_install_prefix  ${CMAKE_INSTALL_PREFIX})
    set(hdf5_install_prefix    ${CMAKE_INSTALL_PREFIX})
endif()



if(H5PP_PRINT_INFO)
    message("=========================== h5pp build options ==============================")
    message(STATUS "BUILD_SHARED_LIBS       : ${BUILD_SHARED_LIBS}")
    message(STATUS "BUILD_EXAMPLES          : ${BUILD_EXAMPLES}")
    message(STATUS "ENABLE_TESTS            : ${ENABLE_TESTS}")
    message(STATUS "H5PP_IS_SUBPROJECT      : ${H5PP_IS_SUBPROJECT}")
    message(STATUS "DOWNLOAD_METHOD         : ${DOWNLOAD_METHOD}")
    message(STATUS "APPEND_LIBSUFFIX        : ${APPEND_LIBSUFFIX}")
    message(STATUS "PREFER_CONDA_LIBS       : ${PREFER_CONDA_LIBS}")
    message(STATUS "CMAKE_INSTALL_PREFIX    : ${CMAKE_INSTALL_PREFIX}")
    message("")
endif()




#########################################################################
### Define main target and auxiliary for partial consumption          ###
#########################################################################
add_library(h5pp INTERFACE)
add_library(headers INTERFACE)
add_library(deps INTERFACE)
add_library(flags INTERFACE)



#################################
### Compiler and linker flags ###
#################################
target_compile_features(flags INTERFACE cxx_std_17)
target_link_libraries (flags INTERFACE stdc++fs)
if(GCC_TOOLCHAIN)
    target_compile_options(flags INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
endif()

target_compile_definitions(deps INTERFACE DOWNLOAD_METHOD=${DOWNLOAD_METHOD})

#######################################################
###  Check  #include<filesystem> or                 ###
###  or     #include<experimental/filesystem>       ###
#######################################################
include(${PROJECT_SOURCE_DIR}/cmake/CheckCXXFilesystem.cmake)
CheckCXXFilesystem()

#######################################################
###  Check  #include<experimental/type_traits>      ###
#######################################################
include(${PROJECT_SOURCE_DIR}/cmake/CheckTypeTraits.cmake)
CheckTypeTraits()

#######################################################
###  Check  #include<optional> or                   ###
###  or     #include<experimental/optional>         ###
#######################################################
include(${PROJECT_SOURCE_DIR}/cmake/CheckCXXOptional.cmake)
CheckCXXOptional()





include(GNUInstallDirs)
target_include_directories(
        headers
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/h5pp/source/include>
        $<INSTALL_INTERFACE:${h5pp_relative_prefix}${CMAKE_INSTALL_INCLUDEDIR}>
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
### Preempt Threads::Threads                                   ###
### It's looked for in dependencies, so we make it right       ###
### before it's done wrong, i.e. with pthread instead of       ###
### -lpthread. If this is undesirable you can preempt it       ###
### yourself similarly                                         ###
##################################################################
if(NOT TARGET Threads::Threads)
    set(CMAKE_THREAD_PREFER_PTHREAD FALSE)
    set(THREADS_PREFER_PTHREAD_FLAG FALSE)
    find_package(Threads)
    set_target_properties(Threads::Threads PROPERTIES INTERFACE_LINK_LIBRARIES pthread)
endif()



##################################################################
### Try to find or fetch all dependencies                      ###
##################################################################
if(NOT "${DOWNLOAD_METHOD}" MATCHES "none")
    if("${DOWNLOAD_METHOD}" MATCHES "conan")
        include(cmake/SetupDependenciesConan.cmake)
    elseif("${DOWNLOAD_METHOD}" MATCHES "native")
        include(cmake/SetupDependenciesNative.cmake)
    endif()
endif()

target_link_libraries(h5pp INTERFACE headers deps flags)

##################################################################
### Print summary of CMake configuration                       ###
##################################################################
if(H5PP_PRINT_INFO)
    message("========================== h5pp target summary ==============================")
    include(${PROJECT_SOURCE_DIR}/cmake/PrintTargetInfo.cmake)
    print_target_info(h5pp)
    print_target_info(headers)
    print_target_info(deps)
    print_target_info(flags)
    print_target_info(spdlog::spdlog)
    print_target_info(Eigen3::Eigen)
    print_target_info(hdf5::hdf5)
    print_target_info(CONAN_PKG::spdlog)
    print_target_info(CONAN_PKG::HDF5)
    print_target_info(CONAN_PKG::Eigen3)
    print_target_info(CONAN_PKG::fmt)
    print_target_info(CONAN_PKG::ZLIB)
    print_target_info(Threads::Threads)
    message("")
endif()




if(H5PP_IS_SUBPROJECT)
    add_library(h5pp::h5pp ALIAS h5pp)
    add_library(h5pp::headers ALIAS headers)
    add_library(h5pp::deps ALIAS deps)
    add_library(h5pp::flags ALIAS h5pp)
else()

    #######################################################
    ### Install library                                 ###
    ### Default path is ${PROJECT_SOURCE_DIR}/install   ###
    ### where ${PROJECT_SOURCE_DIR}  is the path of     ###
    ### this CMakeLists.txt                             ###
    #######################################################
    # Read about this share path here https://cmake.org/cmake/help/v3.12/command/find_package.html
    set(H5PP_CONFIG_DIR ${h5pp_relative_prefix}share/h5pp/cmake)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/h5pp/source/include/ DESTINATION ${h5pp_relative_prefix}${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)
    install(TARGETS h5pp headers deps flags EXPORT h5ppTargets)

    #Export the targets to a script
    install(EXPORT
            h5ppTargets
            NAMESPACE h5pp::
            DESTINATION ${H5PP_CONFIG_DIR})


    #Create a ConfigVersion.cmake file
    include(CMakePackageConfigHelpers)


    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/h5ppConfig.cmake.in
            ${PROJECT_BINARY_DIR}/h5ppConfig.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/h5ppConfig.deps-native.cmake.in
            ${PROJECT_BINARY_DIR}/h5ppConfig.deps-native.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/h5ppConfig.deps-conan.cmake.in
            ${PROJECT_BINARY_DIR}/h5ppConfig.deps-conan.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    write_basic_package_version_file(
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
    )


    ##Install the config, configversion and custom find modules
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps-native.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfig.deps-conan.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/h5ppConfigVersion.cmake
            ${PROJECT_SOURCE_DIR}/cmake/FindEigen3.cmake
            ${PROJECT_SOURCE_DIR}/cmake/Findspdlog.cmake
            ${PROJECT_SOURCE_DIR}/cmake/FindPackageHDF5.cmake
            DESTINATION ${H5PP_CONFIG_DIR}
            COMPONENT config
            )



    if(DOWNLOAD_METHOD MATCHES "conan")
        # Install conan related files to define dependency targets the same way
        # we've already done here.
        set(CONAN_INSTALL_FILES
                ${PROJECT_SOURCE_DIR}/conanfile.txt
                ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.txt
                ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt
                )
        foreach(conan_file ${CONAN_INSTALL_FILES})
            if(EXISTS "${conan_file}")
                install(FILES ${conan_file}
                        DESTINATION ${H5PP_CONFIG_DIR}
                        COMPONENT config)
            endif()
        endforeach()

    endif()
endif()

##################################################################
### Unit testing                                               ###
##################################################################
if(NOT H5PP_IS_SUBPROJECT AND ENABLE_TESTS)
    if(ALL_TARGETS_FOUND)
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

    elseif(NOT "${DOWNLOAD_METHOD}" MATCHES "none")
        message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
    endif()
endif()




#############################
# ADD TARGETS FOR  EXAMPLES #
#############################
if(NOT H5PP_IS_SUBPROJECT AND BUILD_EXAMPLES)
    if(ALL_TARGETS_FOUND)
        add_subdirectory(h5pp/examples/helloworld      ${CMAKE_BINARY_DIR}/h5pp-examples/helloworld)
        add_subdirectory(h5pp/examples/writescalar     ${CMAKE_BINARY_DIR}/h5pp-examples/writescalar)
        add_subdirectory(h5pp/examples/readme-example1 ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example1)
        add_subdirectory(h5pp/examples/readme-example2 ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example2)
        add_subdirectory(h5pp/examples/readme-example3 ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example3)
        add_subdirectory(h5pp/examples/readme-example4 ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example4)


        install(PROGRAMS
                ${CMAKE_BINARY_DIR}/h5pp-examples/helloworld/helloworld
                ${CMAKE_BINARY_DIR}/h5pp-examples/writescalar/writescalar
                ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example1/readme-example1
                ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example2/readme-example2
                ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example3/readme-example3
                ${CMAKE_BINARY_DIR}/h5pp-examples/readme-example4/readme-example4
                DESTINATION ${h5pp_install_prefix}/share/h5pp/examples
                COMPONENT examples)
    elseif(NOT "${DOWNLOAD_METHOD}" MATCHES "none")
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