#!/bin/bash

# Add the conan repository where h5pp is
conan remote add conan-h5pp https://api.bintray.com/conan/davidace/conan-public

# Install h5pp. Use your own profile
conan install h5pp/1.7.2@davidace/stable --profile default