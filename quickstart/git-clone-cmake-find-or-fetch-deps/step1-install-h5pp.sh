#!/bin/bash

# Get the source
git clone -b dev https://github.com/DavidAce/h5pp.git h5pp-source

# Run CMake configure (optionally do this with cmake-gui)
# CMake version > 3.19 is required!
# h5pp will be installed side by side with the project directory
# H5PP_PACKAGE_MANAGER=find-or-cmake will finds dependencies locally, or build them from scratch if missing.

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=h5pp-install \
      -DBUILD_SHARED_LIBS=OFF \
      -DH5PP_PACKAGE_MANAGER=find-or-cmake \
      -DH5PP_ENABLE_EIGEN3=ON \
      -DH5PP_ENABLE_SPDLOG=ON \
      -DH5PP_ENABLE_TESTS=ON \
      -DH5PP_PRINT_INFO=ON \
      -S h5pp-source \
      -B h5pp-build

# Install the headers and other files to the location specified in CMAKE_INSTALL_PREFIX
cmake --build h5pp-build --parallel 4 --config Release  --target install

# Run the tests to make sure everything is in order. This step is optional.
ctest -C Release --output-on-failure


