cmake_minimum_required(VERSION 3.10)
project(write_scalar)


set(CMAKE_VERBOSE_MAKEFILE OFF)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake-modules)

add_executable(${PROJECT_NAME} write_scalar.cpp)

find_package(h5pp HINTS ../../install REQUIRED)


if (h5pp_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp)
endif()