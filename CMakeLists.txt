 
cmake_minimum_required(VERSION 3.10)
project(h5pp VERSION 1.5.0
        DESCRIPTION "A C++17 wrapper for HDF5"
        HOMEPAGE_URL "https://github.com/DavidAce/h5pp"
        )

# Set options for compilation
option(BUILD_SHARED_LIBS "Builds shared libraries" OFF)
option(BUILD_EXAMPLES "Builds examples" OFF)
option(ENABLE_TESTS "Enable testing" OFF)
option(H5PP_IS_SUBPROJECT "Use h5pp with add_subdirectory()" OFF)
option(APPEND_LIBSUFFIX "Append <libname> to CMAKE_INSTALL_PREFIX" OFF)
option(PREFER_CONDA_LIBS "Search for dependencies from anaconda first" OFF)
option(H5PP_PRINT_INFO "Print info during cmake configuration" OFF)
option(H5PP_ENABLE_EIGEN3 "Enables Eigen3 linear algebra library" OFF)
option(H5PP_ENABLE_SPDLOG "Enables Spdlog for logging h5pp internal info to stdout" OFF)
option(H5PP_ENABLE_MPI "Enables use of MPI (work in progress)" OFF)

# Make an "enum" for valid download methods: none find-only native conan
set(VALID_DOWNLOAD_METHODS none find-only native conan)
set(DOWNLOAD_METHOD none CACHE STRING "Download method for external dependencies")
set_property(CACHE DOWNLOAD_METHOD PROPERTY STRINGS ${VALID_DOWNLOAD_METHODS})
if (NOT DOWNLOAD_METHOD IN_LIST VALID_DOWNLOAD_METHODS)
    message(FATAL_ERROR "DOWNLOAD_METHOD must be one of ${VALID_DOWNLOAD_METHODS}")
endif ()

# Print host properties
if (H5PP_PRINT_INFO)
    cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
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



# Used when h5pp is included as subproject (e.g., as Git submodule/subtree) in the source
# tree of a project that uses it. Users may set the option H5PP_IS_SUBPROJECT
# before add_subdirectory(h5pp)
if (NOT H5PP_IS_SUBPROJECT)
    if ("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
        set(H5PP_IS_SUBPROJECT OFF)
    else ()
        message(STATUS "Detected usage as subproject")
        set(H5PP_IS_SUBPROJECT ON)
    endif ()
endif ()


# Let cmake find our Find<package>.cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


# Set default install directory for h5pp and its dependencies
# Append directory <libname> to CMAKE_INSTALL_PREFIX
# Useful if you want to have separate directories for each libs
# and to easily delete them individually
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT H5PP_IS_SUBPROJECT)
    message(STATUS "Setting default install prefix -- CMAKE_INSTALL_PREFIX --> ${CMAKE_BINARY_DIR}/install")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "..." FORCE)
endif ()
if (APPEND_LIBSUFFIX)
    set(H5PP_INSTALL_DIR h5pp/)
    set(H5PP_CONFIG_DIR h5pp/share/h5pp/cmake)
    set(eigen3_install_prefix ${CMAKE_INSTALL_PREFIX}/Eigen3)
    set(spdlog_install_prefix ${CMAKE_INSTALL_PREFIX}/spdlog)
    set(hdf5_install_prefix ${CMAKE_INSTALL_PREFIX}/hdf5)
    set(ghcfilesystem_install_prefix ${CMAKE_INSTALL_PREFIX}/ghcFilesystem)
else ()
    set(H5PP_CONFIG_DIR share/h5pp/cmake)
    set(eigen3_install_prefix ${CMAKE_INSTALL_PREFIX})
    set(spdlog_install_prefix ${CMAKE_INSTALL_PREFIX})
    set(hdf5_install_prefix ${CMAKE_INSTALL_PREFIX})
    set(ghcfilesystem_install_prefix ${CMAKE_INSTALL_PREFIX})
endif ()



if(H5PP_PRINT_INFO)
    message("=========================== h5pp build options ==============================")
    message(STATUS "BUILD_SHARED_LIBS       : ${BUILD_SHARED_LIBS}")
    message(STATUS "BUILD_EXAMPLES          : ${BUILD_EXAMPLES}")
    message(STATUS "ENABLE_TESTS            : ${ENABLE_TESTS}")
    message(STATUS "H5PP_ENABLE_EIGEN3      : ${H5PP_ENABLE_EIGEN3}")
    message(STATUS "H5PP_ENABLE_SPDLOG      : ${H5PP_ENABLE_SPDLOG}")
    message(STATUS "H5PP_ENABLE_MPI         : ${H5PP_ENABLE_MPI}")
    message(STATUS "H5PP_IS_SUBPROJECT      : ${H5PP_IS_SUBPROJECT}")
    message(STATUS "DOWNLOAD_METHOD         : ${DOWNLOAD_METHOD}")
    message(STATUS "APPEND_LIBSUFFIX        : ${APPEND_LIBSUFFIX}")
    message(STATUS "PREFER_CONDA_LIBS       : ${PREFER_CONDA_LIBS}")
    message(STATUS "CMAKE_INSTALL_PREFIX    : ${CMAKE_INSTALL_PREFIX}")
    message("")
endif ()


# Define main target and auxiliary for partial consumption
add_library(h5pp INTERFACE)
add_library(headers INTERFACE)
add_library(deps INTERFACE)
add_library(flags INTERFACE)


########################## IMPORTANT #############################
### Preempt Threads::Threads                                   ###
### It's looked for in dependency configs, so we make it right ###
### before it's done wrong, i.e. with pthread instead of       ###
### -lpthread. If this is undesirable you can preempt it       ###
### yourself similarly. The reason is that downstream I often  ###
### need to avoid a static linking segfault by using:          ###
### -Wl, -whole-archive pthread -Wl, -no-whole-archive         ###
### and cmake is unable to identify "-lpthread" as being the   ###
### same as pthread --> multiple declaration errors. Note that ###
### CMake changes it into                                      ###
### -Wl, -whole-archive -lpthread -Wl, -no-whole-archive       ###
### anyway. If an entire dependency tree had only "-lpthread"  ###
### or only "pthread" this wouldn't be a problem, but          ###
### unfortunately different dependencies do it differently.    ###
### Since conan prefers "pthread", I'll do the same here.      ###
##################################################################
### Read more about pthread segfault
### https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
if (NOT TARGET Threads::Threads)
    set(CMAKE_THREAD_PREFER_PTHREAD FALSE)
    set(THREADS_PREFER_PTHREAD_FLAG FALSE)
    find_package(Threads)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_target_properties(Threads::Threads PROPERTIES INTERFACE_LINK_LIBRARIES pthread)
    endif ()
endif ()

if (H5PP_ENABLE_MPI AND NOT WIN32)
    find_package(MPI)
    if (TARGET MPI::MPI_CXX)
        target_link_libraries(flags INTERFACE MPI::MPI_CXX)
    endif ()
endif ()
# On some HPC clusters Clang needs path to gnu gcc toolchain because it's not in path
if (GCC_TOOLCHAIN AND ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    target_compile_options(flags INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
endif ()

# Add def for download method for use in installed h5ppConfig.cmake
target_compile_definitions(deps INTERFACE DOWNLOAD_METHOD=${DOWNLOAD_METHOD})


# h5pp requires the filesystem header (and possibly stdc++fs library)
find_package(Filesystem COMPONENTS Final Experimental)
if (TARGET std::filesystem)
    target_link_libraries(flags INTERFACE std::filesystem)
elseif (DOWNLOAD_METHOD MATCHES "find|none")
    message(STATUS "Your compiler lacks std::filesystem. Set DOWNLOAD_METHOD to 'conan' or 'native' to get the ghc::filesystem replacement")
    message(STATUS "Read more about ghc::filesystem here: https://github.com/gulrak/filesystem")
    message(FATAL_ERROR "<filesystem> header and/or library not found")
else ()
    message(STATUS "Your compiler lacks std::filesystem. Getting ghc::filesystem replacement")
    message(STATUS "Read more about ghc::filesystem here: https://github.com/gulrak/filesystem")
    include(cmake/Fetch_ghcFilesystem.cmake)
    target_link_libraries(flags INTERFACE ghcFilesystem::ghc_filesystem)
    list(APPEND H5PP_DIRECTORY_HINTS ${ghcfilesystem_install_prefix})
endif ()


# Check  #include<optional> or #include<experimental/optional>
include(${PROJECT_SOURCE_DIR}/cmake/CheckCXXOptional.cmake)
CheckCXXOptional()

# Required compiler features
target_compile_features(flags INTERFACE cxx_std_17)
target_compile_options(flags INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive->) # Need this for and/or logical operators on VS


target_include_directories(headers INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/h5pp/include>
        $<INSTALL_INTERFACE:${H5PP_INSTALL_DIR}include>
        )


# Setup dependency paths
if(PREFER_CONDA_LIBS)
    # Prepend conda dirs for hints when using find_package
    list(APPEND H5PP_DIRECTORY_HINTS
            $ENV{CONDA_PREFIX}
            $ENV{HOME}/anaconda3
            $ENV{HOME}/miniconda
            $ENV{HOME}/.conda
            )
endif()
list(APPEND H5PP_DIRECTORY_HINTS ${CMAKE_INSTALL_PREFIX})


#Try to find or fetch all dependencies
if (NOT DOWNLOAD_METHOD MATCHES "none")
    if (DOWNLOAD_METHOD MATCHES "conan")
        include(cmake/SetupDependenciesConan.cmake)
    else ()
        include(cmake/SetupDependenciesNative.cmake)
    endif ()
endif ()

# Link all targets to one main h5pp target (the only one that users should need)
# However, it's nice to have them separately also if need be.
target_link_libraries(h5pp INTERFACE headers deps flags)


# Print summary of CMake configuration
if (H5PP_PRINT_INFO)
    message("========================== h5pp target summary ==============================")
    include(${PROJECT_SOURCE_DIR}/cmake/PrintTargetInfo.cmake)
    print_target_info(h5pp)
    print_target_info(headers)
    print_target_info(deps)
    print_target_info(flags)
    print_target_info(std::filesystem)
    print_target_info(ghcFilesystem::ghc_filesystem)
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

    # Install library
    # Read about this share path here https://cmake.org/cmake/help/v3.12/command/find_package.html
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/h5pp/include/ DESTINATION ${H5PP_INSTALL_DIR}include COMPONENT headers)
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
            ${CMAKE_BINARY_DIR}/h5ppConfig.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/h5ppConfig.deps-native.cmake.in
            ${CMAKE_BINARY_DIR}/h5ppConfig.deps-native.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/h5ppConfig.deps-conan.cmake.in
            ${CMAKE_BINARY_DIR}/h5ppConfig.deps-conan.cmake
            INSTALL_DESTINATION ${H5PP_CONFIG_DIR}
            PATH_VARS H5PP_CONFIG_DIR H5PP_DIRECTORY_HINTS
    )

    write_basic_package_version_file(
            ${CMAKE_BINARY_DIR}/h5ppConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
    )


    ##Install the config, configversion and custom find modules
    install(FILES
            ${CMAKE_BINARY_DIR}/h5ppConfig.cmake
            ${CMAKE_BINARY_DIR}/h5ppConfig.deps-native.cmake
            ${CMAKE_BINARY_DIR}/h5ppConfig.deps-conan.cmake
            ${CMAKE_BINARY_DIR}/h5ppConfigVersion.cmake
            ${PROJECT_SOURCE_DIR}/cmake/FindEigen3.cmake
            ${PROJECT_SOURCE_DIR}/cmake/Findspdlog.cmake
            ${PROJECT_SOURCE_DIR}/cmake/FindPackageHDF5.cmake
            ${PROJECT_SOURCE_DIR}/cmake/FindFilesystem.cmake
            DESTINATION ${H5PP_CONFIG_DIR}
            COMPONENT config
            )



    if(DOWNLOAD_METHOD MATCHES "conan")
        # Install conan related files to define dependency targets the same way
        # we've already done here.
        set(CONAN_INSTALL_FILES
                ${PROJECT_SOURCE_DIR}/conanfile.txt
                ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake
                ${CMAKE_BINARY_DIR}/conanbuildinfo.txt
                ${CMAKE_BINARY_DIR}/conaninfo.txt
                )
        foreach(conan_file ${CONAN_INSTALL_FILES})
            if(EXISTS "${conan_file}")
                install(FILES ${conan_file}
                        DESTINATION ${H5PP_CONFIG_DIR}
                        COMPONENT config)
            endif ()
        endforeach ()

    endif ()
endif ()


# Simple testing with ctest
if (NOT H5PP_IS_SUBPROJECT AND ENABLE_TESTS AND TARGET h5pp)
    enable_testing()
    add_subdirectory(tests/simpleWrite h5pp-tests/simpleWrite)
    add_subdirectory(tests/largeWrite h5pp-tests/largeWrite)
    add_subdirectory(tests/overWrite h5pp-tests/overWrite)
    add_subdirectory(tests/rawArrayWrapper h5pp-tests/rawArrayWrapper)
    add_subdirectory(tests/readWrite h5pp-tests/readWrite)
    add_subdirectory(tests/readWriteAttributes h5pp-tests/readWriteAttributes)
    add_subdirectory(tests/copySwap h5pp-tests/copySwap)
    add_custom_target(all-tests
            DEPENDS test-simpleWrite
            test-largeWrite
            test-overWrite
            test-rawArrayWrapper
            test-readWrite
            test-readWriteAttributes
            test-copySwap)
    # A handy target "all-tests" which runs all test when built.
    add_custom_command(
            TARGET all-tests
            POST_BUILD
            COMMENT "Running Tests"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS h5pp deps flags
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure)

elseif (NOT DOWNLOAD_METHOD MATCHES "none")
    message(STATUS "Dependencies missing, ignoring ENABLE_TESTS=ON")
endif ()


# Build examples
if (NOT H5PP_IS_SUBPROJECT AND BUILD_EXAMPLES AND TARGET h5pp)
    add_subdirectory(examples/helloworld h5pp-examples/helloworld)
    add_subdirectory(examples/writescalar h5pp-examples/writescalar)
    add_subdirectory(examples/readme-example1 h5pp-examples/readme-example1)
    add_subdirectory(examples/readme-example2 h5pp-examples/readme-example2)
    add_subdirectory(examples/readme-example3 h5pp-examples/readme-example3)
    add_subdirectory(examples/readme-example4 h5pp-examples/readme-example4)
    # A handy target "all-examples" to build all examples
    add_custom_target(all-examples DEPENDS
            helloworld
            writescalar
            readme-example1
            readme-example2
            readme-example3
            readme-example4)
elseif (NOT DOWNLOAD_METHOD MATCHES "none")
    message(STATUS "Dependencies missing, ignoring BUILD_EXAMPLES=ON")
endif ()


# Use CPACK to generate .deb install file
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL headers config)

# Define apt dependencies that work with this library
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libhdf5-dev (>=1.10), libeigen3-dev (>=3.3.4), libspdlog-dev (>=1.3)")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A C++ wrapper for HDF5")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/DavidAce/h5pp")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DavidAce <aceituno@kth.se>")
set(CPACK_DEBIAN_PACKAGE_NAME "h5pp")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_GENERATOR "DEB")
include(CPack)
