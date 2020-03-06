cmake_minimum_required(VERSION 3.12)
project(external-ghcFilesystem)

include(ExternalProject)
ExternalProject_Add(external_ghcFilesystem
    URL         https://github.com/gulrak/filesystem/archive/v1.2.10.tar.gz
    PREFIX      ${CMAKE_BINARY_DIR}
    INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        -DGHC_FILESYSTEM_BUILD_TESTING:BOOL=OFF
        -DGHC_FILESYSTEM_BUILD_EXAMPLES:BOOL=OFF
        -DGHC_FILESYSTEM_WITH_INSTALL:BOOL=ON
        -DCMAKE_INSTALL_MESSAGE=NEVER #Avoid unnecessary output to console
)
