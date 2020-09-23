#!/bin/bash

# Make sure to install the latest version conan and configure your profile before continuing
# For instance, you may need to add `compiler.cppstd=17` under [settings] in ~/.conan/profile/default

cd MyProject
mkdir -p build/Release
cd build/Release


# Run CMake configure (optionally do this with cmake-gui)
# CMake takes care of launching conan and installing dependencies
cmake -DCMAKE_BUILD_TYPE=Release  ../../

# Build the executable
cmake --build . --parallel

# Run the executable
./MyProjectExecutable
