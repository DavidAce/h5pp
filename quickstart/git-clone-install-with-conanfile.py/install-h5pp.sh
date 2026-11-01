#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

# Requires Conan >= 2.0.5 and a configured default profile.
# Run `conan profile detect` first if no default profile exists.
git clone --branch v2 https://github.com/DavidAce/h5pp.git h5pp-source
cd h5pp-source
conan create . --profile=default --build=missing -s:h compiler.cppstd=20
