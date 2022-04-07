#!/bin/bash

# Get the source
git clone -b dev https://github.com/DavidAce/h5pp.git h5pp-source

# Run CMake configure (optionally do this with cmake-gui)
# CMake version > 3.15 is required!
# h5pp will be installed side by side with the project directory
# H5PP_PACKAGE_MANAGER=none will skip finding dependencies

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=h5pp-install \
      -DBUILD_SHARED_LIBS=OFF \
      -DH5PP_PACKAGE_MANAGER=none \
      -DH5PP_ENABLE_EIGEN3=ON \
      -DH5PP_ENABLE_SPDLOG=ON \
      -DH5PP_ENABLE_TESTS=OFF \
      -DH5PP_PRINT_INFO=ON \
      -S h5pp-source \
      -B h5pp-build


# Install the headers and other files to the location specified in CMAKE_INSTALL_PREFIX
cmake --build h5pp-build --parallel 4 --config Release  --target install


