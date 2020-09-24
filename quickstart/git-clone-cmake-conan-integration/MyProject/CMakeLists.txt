cmake_minimum_required(VERSION 3.12)
project(MyProject VERSION 0.0.1 DESCRIPTION "This example uses h5pp as a dependency")

# Create an executable
add_executable(MyProjectExecutable source/main.cpp)

# Enable C++17
target_compile_features(MyProjectExecutable PRIVATE cxx_std_17)


# Download Conan-CMake integration automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(
        CONANFILE conanfile.txt
        SETTINGS compiler.cppstd=17
        BUILD_TYPE ${CMAKE_BUILD_TYPE}
        BASIC_SETUP CMAKE_TARGETS
	    PROFILE_AUTO ALL
        BUILD missing)

message(STATUS "Found Conan targets: ${CONAN_TARGETS}")
target_link_libraries(MyProjectExecutable PRIVATE ${CONAN_TARGETS})
