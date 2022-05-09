#!/bin/bash

# Make sure to install the latest version conan and configure your profile before continuing
# For instance, you may need to add `compiler.cppstd=17` under [settings] in ~/.conan/profile/default

# This command reads conanfile.txt inside MyProject/ and generates Find<Pkg>.cmake files.
# These are put into MyProject-build ready to consume by our project.
# Note that conan generates those files because MyProject/conanfile.txt has:
#   [generator]
#   CMakeDeps

conan install --profile=default --build=missing --install-folder=MyProject-build MyProject

# Run CMake configure (optionally do this with cmake-gui)
# Note that we set CMAKE_MODULE_PATH to the full path where conan has put all the Find<Pkg>.cmake files
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH:PATH="$(pwd)"/MyProject-build \
      -DBUILD_SHARED_LIBS=OFF \
      -S MyProject \
      -B MyProject-build


# Build the executable
cmake --build MyProject-build --parallel 4

# Run the executable
./MyProject-build/MyProjectExecutable
