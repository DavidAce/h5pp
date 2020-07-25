#!/bin/bash

# Get the source
git clone https://github.com/DavidAce/h5pp.git

# Make build directories
cd h5pp
mkdir -p build/Release
cd build/Release


# Run CMake configure (optionally do this with cmake-gui)
# CMake version > 3.12 is required!
# The default install directory will be "install" under this current directory.
# For this example we use "../../install" instead.

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=../../install \
      -DBUILD_SHARED_LIBS=OFF \
      -DH5PP_DOWNLOAD_METHOD=none \
      -DH5PP_ENABLE_EIGEN3=ON \
      -DH5PP_ENABLE_SPDLOG=ON \
      -DH5PP_ENABLE_TESTS=OFF \
      -DH5PP_PRINT_INFO=ON \
      ../../


# Install the headers and other files to the location specified in CMAKE_INSTALL_PREFIX
cmake --build . --target install


