#!/bin/bash

cd MyProject
mkdir -p build/Release
cd build/Release


# Run CMake configure (optionally do this with cmake-gui)

cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      ../../

# Build the executable
cmake --build . --parallel 4

# Run the executable
./MyProjectExecutable
