cmake_minimum_required(VERSION 3.10)
project(copySwap)

add_executable(${PROJECT_NAME} copySwap.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/h5pp/source/include)
target_compile_options (${PROJECT_NAME} PRIVATE  -Wall -Wpedantic -Wextra -O0)           ### Release build options
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp h5pp::deps)
target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options  (${PROJECT_NAME} PRIVATE -std=c++17)
target_link_libraries   (${PROJECT_NAME} PRIVATE -lstdc++fs)
add_test(NAME copySwapTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
