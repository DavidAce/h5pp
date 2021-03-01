cmake_minimum_required(VERSION 3.12)
if(H5PP_PRINT_INFO)

    # Print host properties
    cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
    cmake_host_system_information(RESULT _proc_type QUERY PROCESSOR_DESCRIPTION)
    cmake_host_system_information(RESULT _os_name QUERY OS_NAME)
    cmake_host_system_information(RESULT _os_release QUERY OS_RELEASE)
    cmake_host_system_information(RESULT _os_version QUERY OS_VERSION)
    cmake_host_system_information(RESULT _os_platform QUERY OS_PLATFORM)
    message(STATUS "| H5PP BUILD INFO:\n"
            "-- |----------------\n"
            "-- | ${_host_name}\n"
            "-- | ${_os_name} ${_os_platform} ${_os_release}\n"
            "-- | ${_proc_type}\n"
            "-- | ${_os_version}")

    # Print CMake options
    message(STATUS  "|----------------\n"
            "-- | BUILD_SHARED_LIBS       : ${BUILD_SHARED_LIBS}\n"
            "-- | H5PP_BUILD_EXAMPLES     : ${H5PP_BUILD_EXAMPLES}\n"
            "-- | H5PP_ENABLE_TESTS       : ${H5PP_ENABLE_TESTS}\n"
            "-- | H5PP_ENABLE_EIGEN3      : ${H5PP_ENABLE_EIGEN3}\n"
            "-- | H5PP_ENABLE_SPDLOG      : ${H5PP_ENABLE_SPDLOG}\n"
            "-- | H5PP_ENABLE_FMT         : ${H5PP_ENABLE_FMT}\n"
            "-- | H5PP_ENABLE_MPI         : ${H5PP_ENABLE_MPI}\n"
            "-- | H5PP_IS_SUBPROJECT      : ${H5PP_IS_SUBPROJECT}\n"
            "-- | H5PP_PACKAGE_MANAGER    : ${H5PP_PACKAGE_MANAGER}\n"
            "-- | H5PP_DEPS_IN_SUBDIR     : ${H5PP_DEPS_IN_SUBDIR}\n"
            "-- | H5PP_PREFER_CONDA_LIBS  : ${H5PP_PREFER_CONDA_LIBS}\n"
            "-- | H5PP_PRINT_INFO         : ${H5PP_PRINT_INFO}\n"
            "-- | CMAKE_PREFIX_PATH       : ${CMAKE_PREFIX_PATH}\n"
            "-- | CMAKE_INSTALL_PREFIX    : ${CMAKE_INSTALL_PREFIX}\n")
endif ()
