[![Build Status](https://travis-ci.org/DavidAce/h5pp.svg?branch=master)](https://travis-ci.org/DavidAce/h5pp)

# h5pp
h5pp is a C++ wrapper for HDF5 that focuses on simplicity for the end-user. 
## Features
* Standard CMake build, install and linking.
* Automated install and linking of dependencies if desired.
* Support for common data types:
    - `char`,`int`, `float`, `double` in unsigned and long versions.
    - any of the above in std::complex<> form.
    - any of the above in C-style arrays.
    - `std::vector`
    - `std::string`
    - `Eigen` types such as `Matrix`, `Array` and `Tensor` (from the unsupported module), with automatic conversion to/from row-major storage.

## Usage

```c++

#include <iostream>
#include <h5pp/h5pp.h>
using namespace std::complex_literals;

int main() {
    
    // Initialize a file
    h5pp::File file("someFile.h5", "outputDir");

    // Write a vector with std::complex<double>
    std::vector<std::complex<double>> testvector (5, 10.0 + 5.0i);
    file.write_dataset(testvector, "testvector");

    // Write an Eigen matrix with std::complex<double>
    Eigen::MatrixXcd testmatrix (2, 2);
    testmatrix << 1.0 + 2.0i,  3.0 + 4.0i, 5.0 + 6.0i , 7.0 + 8.0i;
    file.write_dataset(testmatrix, "someGroup/testmatrix");


    return 0;
}

```




## Requirements
* C++17 capable compiler with experimental headers. (tested with GCC version >= 7.3 and CLang version >= 6.0)
* CMake 3.10
* Automated dependencies:
    - [**HDF5**](https://support.hdfgroup.org/HDF5/) (tested with version >= 1.10).
    - [**Eigen**](http://eigen.tuxfamily.org) (tested with version >= 3.3.4).
    - [**spdlog**](https://github.com/gabime/spdlog) (tested with version >= 1.3.1)

The build process will attempt to find the libraries above installed on the system.
By default, CMake will emit an error if it can't find the dependencies. For convenience, CMake is able to download and install these dependencies for you into the given install-directory (default: `install-dir/third-party`),
and add these dependencies to the exported target `h5pp::h5pp`.
To enable this automated behavior, see the available build options below.


## Installation
Build the library just as any CMake project:

```bash
    mkdir build
    cd build
    cmake -DDOWNLOAD_ALL=ON ../
    make
    make install
```

By passing the variable `DOWNLOAD_ALL=ON` CMake will download all the dependencies and install them under `install-dir/third-party` if not found in the system. 
By default `ìnstall-dir` will be `project-dir/install`, where `project-dir` is the directory containing the main `CMakeLists.txt` file.

### Build options

The `cmake` step above takes several options, `cmake [-DOPTIONS=var] ../ `:
* `-DCMAKE_INSTALL_PREFIX:PATH=<install-dir>` to specify install directory (default: `project-dir/install/`).
* `-DDOWNLOAD_HDF5:BOOL=<ON/OFF>` to toggle automatic installation of HDF5 (default: `OFF`).
* `-DDOWNLOAD_EIGEN3:BOOL=<ON/OFF>` to toggle automatic installation of HDF5 (default: `OFF`).
* `-DDOWNLOAD_ALL:BOOL=<ON/OFF>` to toggle automatic installation of third-party dependencies (default: `OFF`).
* `-DCMAKE_BUILD_TYPE=Release/Debug` to specify build type (default: `Release`)
* `-DBUILD_SHARED_LIBS:BOOL=<ON/OFF>` to link dependencies with static or shared libraries (default: `OFF`)
* `-DENABLE_TESTS:BOOL=<ON/OFF>` to run ctests after build (default: `ON`).
* `-DBUILD_EXAMPLES:BOOL=<ON/OFF>` to compile the examples after build (default: `OFF`).
* `-DMARCH=<micro-architecture>` to specify compiler micro-architecture (default: `native`)


In addition, the following variables can be set to help guide CMake's `find_package()` to your preinstalled software (no defaults):

* `-DEigen3_DIR:PATH=<path to Eigen3Config.cmake>` 
* `-DEigen3_ROOT_DIR:PATH=<path to Eigen3 install-dir>` 
* `-DEIGEN3_INCLUDE_DIR:PATH=<path to Eigen3 include-dir>`
* `-DHDF5_DIR:PATH=<path to HDF5Config.cmake>` 



### Linking 
#### Using install method
After installing the library it is easily imported using CMake's `find_package()`, just point it to the install directory.
A minimal `CMakeLists.txt` looks like:

```cmake
    cmake_minimum_required(VERSION 3.10)
    project(FooProject)
    
    add_executable(${PROJECT_NAME} foo.cpp)
    
    find_package(h5pp HINTS <path to h5pp-install-dir> REQUIRED)
    
    if (h5pp_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE h5pp::h5pp)
    endif()
```

The target `h5pp::h5pp` will import and enable everything you need to compile with `h5pp`.

#### Without install method (i.e. just copying the header folder)
You will have to manually link the dependencies `hdf5`, `Eigen3` and `splog` to your project.



### Pro-tip: load into Python using h5py
Complex types are not supported natively by HDF5. Still, the storage layout used in `h5pp` makes it easy to read complex types within Python using `h5py`.
As an example, this is how you would load a complex double array:

```python
import h5py
import numpy as np
file  = h5py.File('myFile.h5', 'r')
myComplexArray = np.asarray(file['path-to-Complex-Array'].value.view(dtype=np.complex128))

```