cmake_minimum_required(VERSION 3.24)
project(MyProject VERSION 0.0.1 DESCRIPTION "This example project uses h5pp as a dependency")

# Create an executable
add_executable(MyProjectExecutable source/main.cpp)

# Find and link h5pp
find_package(h5pp REQUIRED)
target_link_libraries(MyProjectExecutable PRIVATE h5pp::h5pp)
