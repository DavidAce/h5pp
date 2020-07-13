cmake_minimum_required(VERSION 3.12)
project(external-eigen3)
include(ExternalProject)
ExternalProject_Add(external_eigen3
        GIT_REPOSITORY https://github.com/eigenteam/eigen-git-mirror.git
        GIT_TAG 3.3.7
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/Eigen_3.3.7.patch
        PREFIX      ${CMAKE_BINARY_DIR}
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        BUILD_ALWAYS TRUE
        CMAKE_ARGS
        -DCMAKE_POLICY_DEFAULT_CMP0074=NEW
        -DEIGEN_TEST_CXX11:BOOL=ON
        -DCMAKE_EXE_LINKER_FLAGS_INIT=${CMAKE_EXE_LINKER_FLAGS}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_Fortran_COMPILER=${CMAKE_Fortran_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
        )