cmake_minimum_required(VERSION 3.10)
project(test-readWrite)
add_executable(${PROJECT_NAME} readWrite.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE -g -Wpedantic -Wextra -Wall -Wno-ignored-attributes) ### Common build options
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp)
add_test(NAME test-readWrite WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND ${PROJECT_NAME})
