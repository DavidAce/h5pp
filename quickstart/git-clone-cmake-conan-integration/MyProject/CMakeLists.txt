cmake_minimum_required(VERSION 3.18)
project(MyProject VERSION 0.0.1 DESCRIPTION "This example uses h5pp as a dependency")

# Create an executable
add_executable(MyProjectExecutable source/main.cpp)

# Enable C++17
target_compile_features(MyProjectExecutable PRIVATE cxx_std_17)


# Download cmake-conan integrator. You can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.17/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan/conan.cmake"
            EXPECTED_HASH MD5=52a255a933397fdce3d0937f9c737e98
            TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan/conan.cmake)
conan_cmake_autodetect(CONAN_AUTODETECT)
conan_cmake_install(
        BUILD missing outdated cascade
        GENERATOR cmake_find_package_multi
        SETTINGS ${CONAN_AUTODETECT}
        INSTALL_FOLDER ${CMAKE_BINARY_DIR}/conan  # This is where conan will put config files for h5pp and its dependencies
        PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
)
list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/conan) # Let find_package check here first

find_package(h5pp REQUIRED)
target_link_libraries(MyProjectExecutable PRIVATE h5pp::h5pp)
