#!/bin/bash

sudo rm -rf build/Package_home
sudo rm -rf build/Package_system

cmake -E make_directory build/Package_home
cd build/Package_home

cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DENABLE_TESTS=ON \
      -DDOWNLOAD_MISSING=OFF \
      -DCMAKE_INSTALL_PREFIX=~/.h5pp \
      -DAPPEND_LIBSUFFIX=ON \
      -G "CodeBlocks - Unix Makefiles" ../../

cpack
cd ../../

cmake -E make_directory build/Package_system
cd build/Package_system

cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DENABLE_TESTS=ON \
      -DDOWNLOAD_MISSING=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DAPPEND_LIBSUFFIX=OFF \
      -G "CodeBlocks - Unix Makefiles" ../../

sudo cpack
cd ../../
