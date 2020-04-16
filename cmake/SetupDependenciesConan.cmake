
##################################################################
### Install conan-modules/conanfile.txt dependencies          ###
### This uses conan to get spdlog/eigen3/h5pp/ceres           ###
###    eigen/3.3.7@conan/stable                               ###
###    spdlog/1.4.2@bincrafters/stable                        ###
###    hdf5/1.10.5                                            ###
##################################################################


find_program (
        CONAN_COMMAND
        conan
        HINTS ${CONAN_PREFIX} ${CONDA_PREFIX} $ENV{CONAN_PREFIX} $ENV{CONDA_PREFIX}
        PATHS $ENV{HOME}/anaconda3 $ENV{HOME}/miniconda $ENV{HOME}/.conda
        PATH_SUFFIXES bin envs/dmrg/bin
)


# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.14/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    # Let it autodetect libcxx
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # There is no libcxx
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND conan_libcxx compiler.libcxx=libstdc++11)
endif()


if(H5PP_ENABLE_EIGEN3 AND H5PP_ENABLE_SPDLOG AND EXISTS ${PROJECT_SOURCE_DIR}/conanfile.txt)
conan_cmake_run(CONANFILE conanfile.txt
        CONAN_COMMAND ${CONAN_COMMAND}
        SETTINGS compiler.cppstd=17
        SETTINGS "${conan_libcxx}"
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        BASIC_SETUP CMAKE_TARGETS
        BUILD missing)
else()
    set(H5PP_CONAN_PACKAGE_HDF5 hdf5/1.10.5)
    if(H5PP_ENABLE_EIGEN3)
        set(H5PP_CONAN_PACKAGE_EIGEN3 eigen/3.3.7@conan/stable)
    endif()
    if(H5PP_ENABLE_SPDLOG)
        set(H5PP_CONAN_PACKAGE_SPDLOG  spdlog/1.4.2@bincrafters/stable)
    endif()
    conan_cmake_run(
            CONAN_COMMAND ${CONAN_COMMAND}
            SETTINGS compiler.cppstd=17
            SETTINGS "${conan_libcxx}"
            BUILD_TYPE ${CMAKE_BUILD_TYPE}
            REQUIRES ${H5PP_CONAN_PACKAGE_HDF5}
            REQUIRES ${H5PP_CONAN_PACKAGE_EIGEN3}
            REQUIRES ${H5PP_CONAN_PACKAGE_SPDLOG}
            BASIC_SETUP CMAKE_TARGETS
            BUILD missing)

endif()
message(STATUS "CONAN TARGETS: ${CONAN_TARGETS}")
list(APPEND H5PP_POSSIBLE_TARGET_NAMES CONAN_PKG::HDF5 CONAN_PKG::hdf5 CONAN_PKG::Eigen3 CONAN_PKG::eigen CONAN_PKG::spdlog)

foreach(tgt ${H5PP_POSSIBLE_TARGET_NAMES})
    if(TARGET ${tgt})
        message(STATUS "Dependency found: [${tgt}]")
        get_target_property(${tgt}_INCLUDE_DIR ${tgt} INTERFACE_INCLUDE_DIRECTORIES)
        if(${tgt}_INCLUDE_DIR)
            list(APPEND H5PP_DIRECTORY_HINTS ${tgt}_INCLUDE_DIR)
        endif()
    endif()
endforeach()

##################################################################
### Link all the things!                                       ###
##################################################################
target_link_libraries(deps INTERFACE ${CONAN_TARGETS})


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
    foreach(tgt ${CONAN_TARGETS})
        print_target_info(${tgt})
    endforeach()
    print_target_info(Threads::Threads)
    message("")
endif()


