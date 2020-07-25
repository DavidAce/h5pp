#!/bin/bash

git clone https://github.com/DavidAce/h5pp.git

cd h5pp
conan create . --profile=default --build=missing
