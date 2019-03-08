cmake_minimum_required(VERSION 3.10)
project(simpleWrite)
add_executable(${PROJECT_NAME} simpleWrite.cpp )
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/h5pp/source/include)
target_compile_options (${PROJECT_NAME} PRIVATE "${COMMON_OPTIONS}")                                 ### Common options
target_compile_options (${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")               ### Debug build options
target_compile_options (${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")           ### Release build options
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp h5pp::deps)
target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options  (${PROJECT_NAME} PRIVATE -std=c++17)
target_link_libraries   (${PROJECT_NAME} PRIVATE -lstdc++fs)

add_test(NAME simpleWriteTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
