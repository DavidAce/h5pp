#!/bin/bash

# Make sure to install the latest version of conan and configure your profile before continuing
# For instance, you may need to add `compiler.cppstd=17` under [settings] in ~/.conan/profile/default


git clone https://github.com/DavidAce/h5pp.git

cd h5pp
conan create . --profile=default --build=missing
