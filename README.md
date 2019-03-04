# libh5pp
libh5pp is a C++ wrapper for HDF5 that focuses on simplicity for the end-user. 

## Features
* Support for common data types:
    - `ìnt`, `float`, `double` in unsigned and long versions.
    - any of the above types in std::complex<> form.
    - `std::string`
    - `std::vector`
    - `Eigen` types such as `Matrix`, `Array` and `Tensor` (from the unsupported module), with automatic conversion to/from row-major storage.
* Standard CMake build, install and linking. 
* Automated install of dependencies. 

## Usage

```c++
#include <h5pp>

int main()

```

## Installation

## Requirements
* C++17 capable compiler.
* CMake 3.11
* Optional dependencies:
    - [**HDF5**](https://support.hdfgroup.org/HDF5/) (tested with version >= 1.10).
    - [**Eigen**](http://eigen.tuxfamily.org) (tested with version >= 3.3.4).
During the build process the dependency [**spdlog**](https://github.com/gabime/spdlog) will be downloaded and installed in a local subdirectory.



