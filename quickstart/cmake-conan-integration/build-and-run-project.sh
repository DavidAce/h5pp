#!/bin/bash

# Make sure to install the latest version conan and configure your profile before continuing
# For instance, you may need to add `compiler.cppstd=17` under [settings] in ~/.conan/profile/default

cmake --list-presets # To view available presets: choose release-win on windows, release-nix on linux/unix

# Configure
cmake --preset=release

# Build
cmake --build --preset=release --parallel 4

# Run the executable
./build/release/MyProjectExecutable