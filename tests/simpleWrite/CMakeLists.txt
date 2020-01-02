cmake_minimum_required(VERSION 3.10)
project(test-simpleWrite CXX)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_executable(${PROJECT_NAME} simpleWrite.cpp )
target_compile_options  (${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>: -Wno-unused-but-set-variable>) ### Only for GNU compiler
target_compile_options  (${PROJECT_NAME} PRIVATE -Wall -Wno-ignored-attributes -Wno-parentheses)        ### Common build options
target_compile_options  (${PROJECT_NAME} PRIVATE $<$<CONFIG:DEBUG>:-g -Wpedantic -Wextra>)              ### Debug build options
target_compile_options  (${PROJECT_NAME} PRIVATE $<$<CONFIG:RELEASE>:-O3 -DNDEBUG>)                     ### Release build options


target_link_libraries(${PROJECT_NAME} PRIVATE h5pp)
add_test(NAME test-simpleWrite WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
