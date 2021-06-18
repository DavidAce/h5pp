cmake_minimum_required(VERSION 3.18)
include(FetchContent)
FetchContent_Declare(fetch-eigen3
        URL      https://gitlab.com/libeigen/eigen/-/archive/3.4-rc1/eigen-3.4-rc1.tar.gz
        URL_MD5  0839b9721e65d2328fb96eb4290d74cc
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


