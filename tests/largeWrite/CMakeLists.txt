cmake_minimum_required(VERSION 3.10)
project(test-largeWrite)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_executable(${PROJECT_NAME} largeWrite.cpp )

target_compile_options  (${PROJECT_NAME} PRIVATE -Wall -Wno-ignored-attributes -Wno-ignored-attributes) ### Common build options
target_compile_options  (${PROJECT_NAME} PRIVATE $<$<CONFIG:DEBUG>:-g -Wpedantic -Wextra>)              ### Debug build options
target_compile_options  (${PROJECT_NAME} PRIVATE $<$<CONFIG:RELEASE>:-O3 -DNDEBUG>)                     ### Release build options

include(${CMAKE_SOURCE_DIR}/cmake-modules/PrintTargetInfo.cmake)
print_target_info(deps)
print_target_info(Eigen3::Eigen)
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp deps flags)



add_test(NAME test-largeWrite WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
