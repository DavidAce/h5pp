#!/bin/bash

# Requires CMake >= 3.26, Conan >= 2.0.5, and a configured default profile.
cmake --list-presets

# Configure: cmake-conan calls Conan on the first find_package().
cmake --preset release

# Build and run the executable.
cmake --build --preset release --parallel 6
./build/release/MyProjectExecutable
