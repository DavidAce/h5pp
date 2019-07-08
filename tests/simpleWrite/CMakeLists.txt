cmake_minimum_required(VERSION 3.10)
project(simpleWrite)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_executable(${PROJECT_NAME} simpleWrite.cpp )

set(COMMON_OPTIONS -g -Wall -Wno-ignored-attributes -Wno-ignored-attributes)
set(DEBUG_OPTIONS       -Wpedantic -Wextra -O0)
set(RELEASE_OPTIONS     -DNDEBUG -O3)
target_compile_options  (${PROJECT_NAME} PRIVATE "${COMMON_OPTIONS}")                                 ### Common build options
target_compile_options  (${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")               ### Debug build options
target_compile_options  (${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")           ### Release build options

target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp h5pp::deps h5pp::flags)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/h5pp/source/include)
add_test(NAME simpleWriteTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
