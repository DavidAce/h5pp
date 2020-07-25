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
      -DH5PP_DOWNLOAD_METHOD=find-or-fetch \
      -DH5PP_ENABLE_EIGEN3=ON \
      -DH5PP_ENABLE_SPDLOG=ON \
      -DH5PP_ENABLE_TESTS=ON \
      -DH5PP_PRINT_INFO=ON \
      ../../

# Build the targets if any were selected.
# In this case, it would only be the enabled tests.
cmake --build . --parallel 4


# Install the headers and other files to the location specified in CMAKE_INSTALL_PREFIX
cmake --build . --target install

# Run the tests to make sure everything is in order. This step is optional.
ctest -C Release --output-on-failure


