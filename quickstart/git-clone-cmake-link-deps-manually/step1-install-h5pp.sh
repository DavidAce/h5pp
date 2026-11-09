#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

# Requires CMake >= 3.26 and preinstalled HDF5 C/HL, Eigen >= 3.4.0,
# fmt >= 9.1.0, and spdlog >= 1.12.0. Use CMAKE_PREFIX_PATH for custom prefixes.
git clone --branch v2 https://github.com/DavidAce/h5pp.git h5pp-source

# No dependency provider: find_package uses the dependencies already installed.
cmake -S h5pp-source -B h5pp-build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
      -DCMAKE_INSTALL_PREFIX="$PWD/h5pp-install" \
      -DH5PP_ENABLE_EIGEN3=ON \
      -DH5PP_ENABLE_SPDLOG=ON \
      -DH5PP_ENABLE_TESTS=OFF
cmake --build h5pp-build --parallel 6 --config Release
cmake --install h5pp-build --config Release
