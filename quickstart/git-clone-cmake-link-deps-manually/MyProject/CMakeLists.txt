cmake_minimum_required(VERSION 3.24)
project(MyProject VERSION 0.0.1 LANGUAGES C CXX)

# h5pp was installed against dependencies supplied by the user. Its installed
# package finds those dependencies again and supplies their CMake targets.
find_package(h5pp 2.0.0 REQUIRED)
add_executable(MyProjectExecutable source/main.cpp)
target_link_libraries(MyProjectExecutable PRIVATE h5pp::h5pp)

# h5pp::h5pp supplies headers, dependencies, and the required C++ standard.
# h5pp::headers, h5pp::deps, and h5pp::flags are available separately when needed.
# Eigen is header-only; its include directory comes from Eigen3::Eigen.
# The self-contained h5pp::hdf5 target supplies HDF5's C and high-level APIs.
