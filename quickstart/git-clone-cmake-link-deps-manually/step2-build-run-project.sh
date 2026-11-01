#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

cmake -S MyProject -B MyProject-build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
      -Dh5pp_ROOT:PATH="$PWD/h5pp-install"
cmake --build MyProject-build --parallel 6 --config Release
./MyProject-build/MyProjectExecutable
