cmake_minimum_required(VERSION 3.10)
project(copySwap)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(${PROJECT_NAME} copySwap.cpp)
################################
### Compiler-dependent linker flags ###
################################
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
    target_compile_options(${PROJECT_NAME} INTERFACE -std=c++17  -g -Wall -Wno-ignored-attributes -Wno-ignored-attributes)
    target_link_libraries (${PROJECT_NAME} INTERFACE -lstdc++fs)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} INTERFACE -stdlib=libstdc++ -std=c++17 -g -Wall -Wno-ignored-attributes)
    target_link_libraries (${PROJECT_NAME} INTERFACE -stdlib=libstdc++ -lstdc++fs)
    if(GCC_TOOLCHAIN)
        target_compile_options(${PROJECT_NAME} INTERFACE --gcc-toolchain=${GCC_TOOLCHAIN})
    endif()
endif()

set(DEBUG_OPTIONS       -Wpedantic -Wextra -O0)
set(RELEASE_OPTIONS     -DNDEBUG -O3)
target_compile_options  (${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")               ### Debug build options
target_compile_options  (${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")           ### Release build options
target_compile_features (${PROJECT_NAME} PRIVATE cxx_std_17)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/h5pp/source/include)
target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp h5pp::deps)
add_test(NAME copySwapTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${PROJECT_NAME})
