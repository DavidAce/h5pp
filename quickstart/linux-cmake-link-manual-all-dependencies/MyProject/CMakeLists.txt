cmake_minimum_required(VERSION 3.12)
project(MyProject VERSION 0.0.1 DESCRIPTION "This example uses h5pp as a dependency")

# Create an executable
add_executable(MyProjectExecutable source/main.cpp)

# Find h5pp installed previously
find_package(h5pp HINTS ${CMAKE_CURRENT_LIST_DIR}/../h5pp/install REQUIRED)

# Link h5pp to the executable
target_link_libraries(MyProjectExecutable PRIVATE h5pp::h5pp)

# This time h5pp was installed without handling of any dependencies.
# We must take care of linking to dependencies ourselves
# Make sure to have installed HDF5, Eigen3 and Spdlog first!
# On Ubuntu, these are installed with
#   sudo apt install libhdf5-dev libeigen3-dev libspdlog-dev

find_package(HDF5 COMPONENTS C HL REQUIRED)
target_link_libraries(MyProjectExecutable PUBLIC ${HDF5_LIBRARIES})
target_include_directories(MyProjectExecutable SYSTEM PUBLIC ${HDF5_INCLUDE_DIRS})


find_package(Eigen3 3.3.4 REQUIRED)
target_include_directories(MyProjectExecutable PUBLIC Eigen3::Eigen})

find_package(spdlog)
target_link_libraries(MyProjectExecutable PUBLIC spdlog::spdlog)


# Note:
#   There are other targets than h5pp::h5pp which allow more control when linking
#   *  `h5pp::h5pp` is the main target including "everything" and should normally be the only target that you need -- headers,flags and (if enabled) the found/downloaded dependencies.
#   *  `h5pp::headers` links the `h5pp` headers only.
#   *  `h5pp::deps` collects library targets to link all the dependencies that were found/downloaded when `h5pp` was built. These can of course be used independently.
#   *  `h5pp::flags` sets compile and linker flags to  enable C++17 and std::filesystem library, i.e. `-std=c++17` and `-lstdc++fs`. On `MSVC` it sets `/permissive-` to enable logical `and`/`or` in C++.
#
#   Specifially for `h5pp::deps`:
#   If `H5PP_DOWNLOAD_METHOD==find-or-fetch|fetch|native` the targets are `Eigen3::Eigen`, `spdlog::spdlog` and `hdf5::hdf5`,
#   If `H5PP_DOWNLOAD_METHOD==conan` the targets are `CONAN_PKG::Eigen3`, `CONAN_PKG::spdlog` and `CONAN_PKG::HDF5`.
#   If `H5PP_DOWNLOAD_METHOD==none` then `h5pp::deps` is empty.


