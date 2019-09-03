cmake_minimum_required(VERSION 3.10)
project(largeWrite)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(${PROJECT_NAME} largeWrite.cpp )

set(COMMON_OPTIONS -g -Wall -Wno-ignored-attributes -Wno-ignored-attributes)
set(DEBUG_OPTIONS       -Wpedantic -Wextra -O0)
set(RELEASE_OPTIONS     -DNDEBUG -O3)
target_compile_options  (${PROJECT_NAME} PRIVATE "${COMMON_OPTIONS}")                                 ### Common build options
target_compile_options  (${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")               ### Debug build options
target_compile_options  (${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")           ### Release build options

if (NOT TARGET h5pp::h5pp)
    find_package(h5pp HINTS ../../install ${h5pp_DIR})
endif()


target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp h5pp::deps h5pp::flags)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/h5pp/source/include)
add_test(NAME largeWriteTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
