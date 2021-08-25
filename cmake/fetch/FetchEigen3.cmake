cmake_minimum_required(VERSION 3.15)
include(FetchContent)
FetchContent_Declare(fetch-eigen3
        URL  https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
        URL_MD5 4c527a9171d71a72a9d4186e65bea559
        QUIET
        CMAKE_ARGS
        -DEIGEN_TEST_CXX11:BOOL=ON
        -DBUILD_TESTING:BOOL=OFF
        -DEIGEN_BUILD_DOC:BOOL=OFF
        -DEIGEN_BUILD_PKGCONFIG:BOOL=OFF
        )
if(NOT TARGET Eigen3::Eigen)
    FetchContent_MakeAvailable(fetch-eigen3) # Avoid needless configure on header only library
    if(NOT TARGET Eigen3::Eigen AND TARGET eigen)
        add_library(Eigen3::Eigen ALIAS eigen)
    endif()
endif()


