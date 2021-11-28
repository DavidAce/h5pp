cmake_minimum_required(VERSION 3.15)

if(H5PP_PACKAGE_MANAGER MATCHES "conan")
    ##################################################################
    ### Install dependencies from conanfile.py                    ###
    ##################################################################

    find_program (CONAN_COMMAND conan HINTS ${H5PP_CONAN_CANDIDATE_PATHS} PATH_SUFFIXES bin envs/dmrg/bin)
    if(NOT CONAN_COMMAND)
        message(FATAL_ERROR "Could not find conan program executable")
    else()
        message(STATUS "Found conan: ${CONAN_COMMAND}")
    endif()

    # Download cmake-conan integrator automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.17/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan/conan.cmake"
                EXPECTED_HASH MD5=52a255a933397fdce3d0937f9c737e98
                TLS_VERIFY ON)
    endif()

    include(${CMAKE_BINARY_DIR}/conan/conan.cmake)
    conan_cmake_autodetect(CONAN_AUTODETECT)
    conan_cmake_install(
            CONAN_COMMAND ${CONAN_COMMAND}
            BUILD missing outdated cascade
            GENERATOR cmake_find_package_multi
            SETTINGS ${CONAN_AUTODETECT}
            INSTALL_FOLDER ${CMAKE_BINARY_DIR}/conan
            PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
    )


    ##################################################################
    ### Link all the things!                                       ###
    ##################################################################
    if(NOT CONAN_CMAKE_SILENT_OUTPUT)
        set(CONAN_CMAKE_SILENT_OUTPUT OFF) # Default is off
    endif()
    list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/conan)
    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/conan)
    # Use CONFIG to avoid MODULE mode. This is recommended for the cmake_find_package_multi generator
    find_package(HDF5 1.12.0 COMPONENTS C HL REQUIRED CONFIG)
    find_package(Eigen3 3.4 REQUIRED CONFIG)
    find_package(spdlog 1.9.2 REQUIRED CONFIG)
    find_package(fmt 8.0.1 REQUIRED CONFIG)
    target_link_libraries(deps INTERFACE HDF5::HDF5 Eigen3::Eigen spdlog::spdlog fmt::fmt)
endif()
