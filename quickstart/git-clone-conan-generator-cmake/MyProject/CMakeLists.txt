cmake_minimum_required(VERSION 3.19)
project(MyProject VERSION 0.0.1 DESCRIPTION "This example uses h5pp as a dependency")

# Create an executable
add_executable(MyProjectExecutable source/main.cpp)

find_package(h5pp REQUIRED)
target_compile_features(MyProjectExecutable PRIVATE cxx_std_17)
target_link_libraries(MyProjectExecutable PRIVATE h5pp::h5pp)
