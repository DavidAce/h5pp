#!/bin/bash

# Run CMake configure (optionally do this with cmake-gui)
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -Dh5pp_ROOT:PATH=h5pp-install \
      -S MyProject \
      -B MyProject-build


# Build the executable
cmake --build MyProject-build --parallel 4

# Run the executable
./MyProject-build/MyProjectExecutable
