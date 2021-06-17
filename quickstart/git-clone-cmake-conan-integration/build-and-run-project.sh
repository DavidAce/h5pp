#!/bin/bash

# Make sure to install the latest version conan and configure your profile before continuing
# For instance, you may need to add `compiler.cppstd=17` under [settings] in ~/.conan/profile/default

# Run CMake configure (optionally do this with cmake-gui)
# CMake takes care of launching conan and installing dependencies
# Run CMake configure (optionally do this with cmake-gui)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -S MyProject \
      -B MyProject-build


# Build the executable
cmake --build MyProject-build --parallel 4

# Run the executable
./MyProject-build/MyProjectExecutable
