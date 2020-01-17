
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
message("CONAN TARGETS: ${CONAN_TARGETS}")

if(TARGET CONAN_PKG::Eigen3)
    target_link_libraries(deps INTERFACE CONAN_PKG::Eigen3)
    get_target_property(EIGEN3_INCLUDE_DIR CONAN_PKG::Eigen3 INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${EIGEN3_INCLUDE_DIR})
    list(APPEND TARGETS_FOUND CONAN_PKG::Eigen3)
else()
    list(APPEND TARGETS_NOT_FOUND CONAN_PKG::Eigen3)
endif()


if(TARGET CONAN_PKG::spdlog)
    target_link_libraries(deps INTERFACE CONAN_PKG::spdlog)
    get_target_property(SPDLOG_INCLUDE_DIR CONAN_PKG::spdlog INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${SPDLOG_INCLUDE_DIR})
    list(APPEND TARGETS_FOUND CONAN_PKG::spdlog)
else()
    list(APPEND TARGETS_NOT_FOUND CONAN_PKG::spdlog)
endif()


if(TARGET CONAN_PKG::HDF5)
    target_link_libraries(deps INTERFACE CONAN_PKG::HDF5)
    get_target_property(HDF5_INCLUDE_DIR CONAN_PKG::HDF5 INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND H5PP_DIRECTORY_HINTS ${HDF5_INCLUDE_DIR})
    list(APPEND TARGETS_FOUND CONAN_PKG::HDF5)
else()
    list(APPEND TARGETS_NOT_FOUND CONAN_PKG::HDF5)
endif()


foreach(tgt ${TARGETS_NOT_FOUND})
    message(STATUS "Dependency not found: [${tgt}]")
endforeach()

if(TARGETS_NOT_FOUND)
    set(ALL_TARGETS_FOUND FALSE)
else()
    set(ALL_TARGETS_FOUND TRUE)
endif()


##################################################################
### Link all the things!                                       ###
##################################################################
target_link_libraries(deps INTERFACE ${TARGETS_FOUND})





