#!/bin/bash

# Add the conan repository where h5pp is
conan remote add conan-h5pp https://api.bintray.com/conan/davidace/conan-public

cd MyProject
mkdir -p build/Release
cd build/Release


# Run CMake configure (optionally do this with cmake-gui)

cmake -DCMAKE_BUILD_TYPE=Release  ../../

# Build the executable
cmake --build . --parallel

# Run the executable
./MyProjectExecutable
