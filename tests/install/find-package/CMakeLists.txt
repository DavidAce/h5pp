cmake_minimum_required(VERSION 3.26)
project(h5pp-install-find-package LANGUAGES C CXX)

find_package(h5pp 2.0 REQUIRED CONFIG)

add_executable(h5pp-install-find-package main.cpp)
target_link_libraries(h5pp-install-find-package PRIVATE h5pp::h5pp)

enable_testing()
add_test(NAME h5pp-install-find-package COMMAND h5pp-install-find-package)
