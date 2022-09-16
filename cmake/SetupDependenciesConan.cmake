cmake_minimum_required(VERSION 3.15)

if(H5PP_PACKAGE_MANAGER MATCHES "conan")
    ##################################################################
    ### Install dependencies from conanfile.py                     ###
    ##################################################################
    unset(CONAN_COMMAND CACHE)
    find_program (CONAN_COMMAND conan PATHS ${H5PP_CONAN_HINTS} PATH_SUFFIXES ${H5PP_CONAN_PATH_SUFFIXES} REQUIRED)

    # Download cmake-conan integrator
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.17/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan/conan.cmake"
                EXPECTED_HASH MD5=52a255a933397fdce3d0937f9c737e98
                TLS_VERIFY ON)
    endif()

    include(${CMAKE_BINARY_DIR}/conan/conan.cmake)

    # Copy the current compiler flags to conan
    string(TOUPPER "${CMAKE_BUILD_TYPE}" CONAN_BUILD_TYPE)
    set(CONAN_CXXFLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CONAN_BUILD_TYPE}}")
    set(CONAN_CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CONAN_BUILD_TYPE}}")
    set(CONAN_LDFLAGS "${CMAKE_EXE_LINKER_FLAGS}")

    conan_cmake_autodetect(CONAN_AUTODETECT)
    conan_cmake_install(
            CONAN_COMMAND ${CONAN_COMMAND}
            BUILD ${CONAN_BUILD} missing outdated cascade
            GENERATOR cmake_find_package_multi
            SETTINGS ${CONAN_AUTODETECT}
            INSTALL_FOLDER ${CMAKE_BINARY_DIR}/conan
            ENV CC=${CMAKE_C_COMPILER} # Fixes issue with CMake not detecting the right compiler when not building from scratch
            ENV CXX=${CMAKE_CXX_COMPILER} # Fixes issue with CMake not detecting the right compiler when not building from scratch
            ENV CXXFLAGS=${CONAN_CXXFLAGS}
            ENV CFLAGS=${CONAN_CFLAGS}
            ENV LDFLAGS=${CONAN_LDFLAGS}
            ${CONAN_OPTIONS}
            PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
    )


    ##################################################################
    ### Find all the things!                                       ###
    ##################################################################
    if(NOT CONAN_CMAKE_SILENT_OUTPUT)
        set(CONAN_CMAKE_SILENT_OUTPUT OFF) # Default is off
    endif()
    list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/conan)
    list(PREPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/conan)
    # Use CONFIG to avoid MODULE mode. This is recommended for the cmake_find_package_multi generator
    find_package(HDF5 1.13.1 COMPONENTS C HL REQUIRED CONFIG)
    find_package(Eigen3 3.4 REQUIRED CONFIG)
    find_package(spdlog 1.10.0 REQUIRED CONFIG)
    find_package(fmt 8.1.1 REQUIRED CONFIG)
    target_link_libraries(deps INTERFACE HDF5::HDF5 Eigen3::Eigen spdlog::spdlog fmt::fmt)
endif()
