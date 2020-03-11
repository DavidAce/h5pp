[![Build Status](https://travis-ci.org/DavidAce/h5pp.svg?branch=master)](https://travis-ci.org/DavidAce/h5pp)
[![Build Status](https://github.com/DavidAce/h5pp/workflows/Actions/badge.svg)](https://github.com/DavidAce/h5pp/actions)
[![Anaconda-Server Badge](https://anaconda.org/davidace/h5pp/badges/installer/conda.svg)](https://conda.anaconda.org/davidace)
[![Download](https://img.shields.io/badge/Install%20with-conan-green)](https://bintray.com/davidace/conan-public/h5pp%3Adavidace/_latestVersion)
[![Download](https://img.shields.io/badge/OS-Linux%7COSX%7CWindows-blue)](https://img.shields.io/badge/OS-Linux%7COSX%7CWindows-blue)

# h5pp
`h5pp` is a C++17 wrapper for HDF5 with focus on simplicity.

In just a few lines of code, `h5pp` lets users read and write to [HDF5](https://www.hdfgroup.org/) files, a portable binary format.
`h5pp` supports common data types and common containers, such as std::vector.
In particular, `h5pp` makes it easy to read and write [**Eigen**](http://eigen.tuxfamily.org) matrices and tensors.

[Latest release](https://github.com/DavidAce/h5pp/releases) 


## Table of Contents
*  [Introduction](#introduction)
*  [Features](#features)
*  [Usage](#usage)
    *  [Example: Writing an std::vector](#example-writing-an-stdvector)
    *  [Debug and logging](#debug-and-logging)
    *  [File permissions](#file-permissions)
    *  [Storage layout](#storage-layout)
    *  [Compression](#compression)
    *  [Load data into Python](#load-data-into-python)
*  [Download](#download)
*  [Requirements](#requirements)
*  [Build and Install](#build-and-install)
    *  [Option 1: Copy the headers](#option-1-copy-the-headers)
    *  [Option 2: Build and install with CMake](#option-2-build-and-install-with-cmake)
    *  [Opt-in automatic dependency installation](#opt-in-automatic-dependency-installation)
*  [Linking](#linking)

## Introduction
[HDF5](https://www.hdfgroup.org/) is a popular format for portable binary storage of large datasets.
With bindings to many languages such as Python, Julia, Matlab and many others,
it is straightforward to export, import and analyze data in a collaborative setting.

In C/C++ using HDF5 directly is not at all straightforward.
Beginners are met with a steep learning curve to the vast API of HDF5.
There are many C/C++ libraries already that simplify the user experience, but as a matter of opinion,
not as simple as what can be found in other languages, like [h5py](https://www.h5py.org/) for Python.

The goal of `h5pp` is to bring this level of simplicity to C++:
*  Users should be able to read/write common C++ data-types in a single line of code.
*  Users should not need prior knowledge of HDF5 for simple tasks.
*  Seemingly simple tasks should stay simple, e.g., specifying storage layout or enabling compression.
*  Advanced tasks should stay possible e.g., specifying chunk dimensions or MPI parallelism.
*  Logs and error messages should be meaningful to beginners.
*  Installation should be just as simple.
 

## Features
*  Header-only C++17 template library
*  Support for common data types:
    *  `short`,`int`,`long`, `long long`, `float`, `double`, `long double` (and unsigned versions)
        *  any of the above in C-style arrays
        *  any of the above in `std::complex<>` form
        *  any of the above in POD-structs with x,y or x,y,z data members. In `h5pp` these go by the name `Scalar2` and `Scalar3`.
            These work well together with types such as `double2` or `float3` found in CUDA.
    *  `std::string` and `char` arrays.
    *  Contiguous containers, such as `std::vector`, with `.data()` methods.
    *  `Eigen` types such as `Matrix`, `Array` and `Tensor`, with automatic conversion to/from row major storage layout.
    *  Any multi-dimensional container with access to a C-style contiguous buffer (without conversion to/from row major).
    *  Support for user-defined compound HDF5 types
    *  Support for creating HDF5 tables from user-defined compound HDF5 types.  
*  Modern CMake build, install and linking using targets.
*  (Opt-in) Automatically find or download dependencies using either [conan package manager](https://conan.io/) or native "CMake-only" methods.


## Usage
Using `h5pp` is intended to be simple. After initializing a file, 
most of the work can be achieved using just two member functions `.writeDataset(...)` and `.readDataset(...)`.

### Example: Writing an `std::vector`
```c++
    #include <h5pp/h5pp.h>
    
    int main() {
        
        // Initialize a file
        h5pp::File file("myDir/someFile.h5");
    
        // Initialize a vector with 10 doubles
        std::vector<double> v (10, 3.14);
        
        // Write the vector to file.
        // Inside the file, the data will be stored in a dataset named "myStdVector"
        file.writeDataset(v, "myStdVector");
        return 0;
    }
```

Find more code examples in the [Wiki](https://github.com/DavidAce/h5pp/wiki).


### File permissions
To define permissions use the settings `AccessMode::<mode>` and/or `CreateMode::<mode>` as arguments when initializing the file.
The possible modes are
*  `AccessMode::`
    *  `READONLY`  Read permission to the file.
    *  `READWRITE` **(default)** Read and write permission to the file.
* `CreateMode::`
    *  `OPEN` Open the file with the given name. Throws an error when file does not exist.
    *  `RENAME` **(default)** File is created with an available file name. If `myFile.h5` already exists, `myFile-1.h5` is created instead. The appended integer is increased until an available name is found
    *  `TRUNCATE` File is created with the given name and erases any pre-existing file. 

The defaults are chosen to avoid loss of data.
To give a concrete example, the syntax works as follows

```c++
    h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE);
```

### Storage Layout
Unless specified, `h5pp` will automatically decide the best storage layout for each dataset. The possible layouts are

* `H5D_COMPACT`:  For scalar or small datasets which can fit in the metadata header. Default on datasets smaller than 32 KB.
* `H5D_CONTIGUOUS`: For medium size datasets.  Default on datasets smaller than 512 KB.
* `H5D_CHUNKED`: For large datasets. Default on datasets larger than 512 KB. This layout has some additional features:
    * Chunking, portioning of the data to improve IO performance by caching more efficiently. Chunk dimensions are calculated by `h5pp` if not given specifically.
    * Compression, disabled by default, and only available if HDF5 was built with zlib enabled.
    * Overwrite with different size (note that the file size never decreases, for instance after overwriting with a smaller dataset).

To specify the layout, pass it as a third argument when writing a new dataset, for instance:

```c++
    file.writeDataset(myData, "science/myChunkedData", H5D_CHUNKED);      // Creates a chunked dataset
```

### Compression
Extendable (or chunked) datasets can also be compressed if HDF5 was built with zlib support. Use these
functions to set or check the compression level:

```c++
    file.setDefaultCompressionLevel(9);            // 0 to 9: 0 to disable compression, 9 for maximum compression.
    file.getDefaultCompressionLevel();             // Gets the current compression level
    h5pp::checkIfCompressionIsAvailable();         // True if your installation of HDF5 has zlib support 
```

or pass a temporary compression level as the fifth argument when writing a dataset:
```c++
    file.writeDataset(myData, "science/myCompressedData", H5D_CHUNKED, std::nullopt, 8); // Creates a chunked dataset with compression level 8.
```



### Debug and logging
[Spdlog](https://github.com/gabime/spdlog) is used to emit debugging information. The amount of console output (verbosity) can be set to any level between `0` and `5`:

* `0: trace` (highest verbosity)
* `1: debug`
* `2: info`  (default)
* `3: warn`
* `4: error`
* `5: critical`  (lowest verbosity)

Set the level when constructing a h5pp::File or by calling the function `.setLogLevel(int)`:

```c++
    int logLevel = 0; // Highest verbosity
    // This way...
    h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::OPEN, logLevel); 
    // or this way
    file.setLogLevel(logLevel);                                                                       
```


### Load data into Python
HDF5 data is easy to load into Python. Loading integer and floating point data is straightforward. compound data is almost as simple.
HDF5 does not support complex types specifically, but `h5pp`enables this through compound HDF5 types. Here is a python example which uses `h5py`
to load 1D arrays from an HDF5 file generated with `h5pp`:

```python
    import h5py
    import numpy as np
    file  = h5py.File('myFile.h5', 'r')
    
    # Originally written as std::vector<double> in h5pp
    myDoubleArray = np.asarray(file['double-array-dataset'])                                     
    
    # Originally written as std::vector<std::complex<double>> in h5pp
    myComplexArray = np.asarray(file['complex-double-array-dataset']).view(dtype=np.complex128) 
```
Notice the cast to `dtype=np.complex128` which interprets each element of the array as two `doubles`, i.e. the real and imaginary parts are `2 * 64 = 128` bits.  



## Download
There are currently 4 ways to obtain `h5pp`:
* `git clone https://github.com/DavidAce/h5pp.git` and install (see below)
* From conda: `conda install -c davidace h5pp`
* From [conan bintray repo](https://bintray.com/davidace/conan-public/h5pp%3Adavidace)
* (Debian only) Download the [latest release](https://github.com/DavidAce/h5pp/releases) and install with apt: `sudo apt install ./h5pp_<version>_amd64.deb` 


## Requirements
* C++17 capable compiler. GCC version >= 7 or Clang version >= 7.0
* CMake version >= 3.12
* [**HDF5**](https://support.hdfgroup.org/HDF5/)  library, version >= 1.8

## Optional dependencies:
* [**Eigen**](http://eigen.tuxfamily.org). Write Eigen matrices and tensors directly. Tested with version >= 3.3.4
* [**spdlog**](https://github.com/gabime/spdlog). Enables logging for debug purposes. Tested with version >= 1.3.1


## Build and install

### Option 1: Copy the headers
Copy the files under `h5pp/source/include` and add `#include<h5pp/h5pp.h>`.
Make sure to compile with `-std=c++17 -lstdc++fs` and link the dependencies `hdf5`, `Eigen3` and `spdlog`. The actual linking
is a non-trivial step, see [linking](#linking) below.


### Option 2: Build and install with CMake
Build the library just as any CMake project. For instance, from the project's root in command-line:

```bash
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=<install-dir> ../
    make
    make install

```

Headers will be installed under `<install-dir>/include` and config files under `<install-dir>/share/h5pp/cmake`.
These config files allow you to use`find_package(h5pp)` in your own projects, which in turn defines the target `h5pp::h5pp` 
with everything you need to link `h5pp` correctly (including dependencies, if you so choose). 
If not set, `CMAKE_INSTALL_PREFIX` defaults to `${CMAKE_BINARY_DIR}/install`, where `${CMAKE_BINARY_DIR}` is the directory you are building from.



#### Opt-in automatic dependency installation
The CMake flag `DOWNLOAD_METHOD` controls the automated behavior for finding or installing dependencies. It can take one of three valid strings:
* `none` (default) all handling of dependencies is disabled and linking is left to the user.
* `find-only` only attempt to find dependencies already installed (no downloads).
* `conan` to install dependencies using the [conan package manager](https://conan.io/). This method is guided by `conanfile.txt` found in this project's root directory.
    This method requires conan to be installed prior (for instance through `pip`, `conda`, `apt`, etc). To let CMake find conan you have three options:
  * Add conan install (or bin) directory to the environment variable `PATH`.
  * Export conan install (or bin) directory in the environment variable `CONAN_PREFIX`, i.e. from command line: `export CONAN_PREFIX=<path-to-conan>` 
  * Give the variable `CONAN_PREFIX` directly to CMake, i.e. from command line: `cmake -DCONAN_PREFIX:PATH=<path-to-conan> ...`
* `native` to find dependencies pre-installed somewhere on your system using `find_package`. If finding fails, dependencies are downloaded and built from source during CMake configure, 
    and installed under `CMAKE_INSTALL_PREFIX`. There are several variables you can pass to CMake to guide `find_package` calls, see [build options](#cmake-build-options). 
    By default it searches standard system install directories as well as typical anaconda3 or miniconda directories.


#### CMake build options

The `cmake` step above takes several options, `cmake [-DOPTIONS=var] ../ `:
* `-DCMAKE_INSTALL_PREFIX:PATH=<install-dir>` to specify install directory (default: `${CMAKE_BINARY_DIR}/install`).
* `-DBUILD_SHARED_LIBS:BOOL=<ON/OFF>` to link dependencies with static or shared libraries (default: `OFF`)
* `-DCMAKE_BUILD_TYPE=Release/Debug` to specify build type of tests and examples (default: `Release`)
* `-DH5PP_ENABLE_TESTS:BOOL=<ON/OFF>` to run ctests after build (recommended!) (default: `OFF`).
* `-DH5PP_BUILD_EXAMPLES:BOOL=<ON/OFF>` to build example programs (default: `OFF`)
* `-DH5PP_DOWNLOAD_METHOD=<none/find-only/conan/native>` to select download method. (default: `none`).
* `-DH5PP_PRINT_INFO:BOOL=<ON/OFF>` to print extra CMake info about the host and generated targets during configure (default: `OFF`).
* `-DH5PP_IS_SUBPROJECT:BOOL<ON/OFF>` Use h5pp with add_subdirectory() (default: `OFF`).
* `-DH5PP_ENABLE_EIGEN3:BOOL=<ON/OFF>` Enables Eigen3 linear algebra library (DEFAULT: `OFF`).
* `-DH5PP_ENABLE_SPDLOG:BOOL=<ON/OFF>` Enables Spdlog for logging h5pp internal info to stdout (DEFAULT: `OFF`).
* `-DH5PP_APPEND_LIBSUFFIX:BOOL=<ON/OFF>` Append a directory with the library name to install directory, i.e. `CMAKE_INSTALL_PREFIX/<libname>/`. This
    is useful when you want to install `h5pp`, `hdf5`, `Eigen3` and `spdlog` in separate folders (default: `OFF`).
* `-DH5PP_PREFER_CONDA_LIBS:BOOL=<ON/OFF>` to prioritize finding dependencies  `hdf5`, `Eigen3` and `spdlog` installed through conda (default: `OFF`).
    Note that this has no effect when `DOWNLOAD_METHOD=conan`.


The following variables can be set to help guide CMake's `find_package` to your pre-installed software (no defaults):

* `-DEigen3_DIR:PATH=<path to Eigen3Config.cmake>` 
* `-DEigen3_ROOT_DIR:PATH=<path to Eigen3 install-dir>` 
* `-DEIGEN3_INCLUDE_DIR:PATH=<path to Eigen3 include-dir>`
* `-DEIGEN3_NO_CMAKE_PACKAGE_REGISTRY:BOOL=<ON/OFF>`
* `-DEIGEN3_NO_DEFAULT_PATH:BOOL=<ON/OFF>`
* `-DEIGEN3_NO_CONFIG:BOOL=<ON/OFF>`
* `-DEIGEN3_CONFIG_ONLY:BOOL=<ON/OFF>`
* `-Dspdlog_DIR:PATH=<path to spdlogConfig.cmake>` 
* `-DSPDLOG_NO_CMAKE_PACKAGE_REGISTRY:BOOL=<ON/OFF>`
* `-DSPDLOG_NO_DEFAULT_PATH:BOOL=<ON/OFF>`
* `-DSPDLOG_NO_CONFIG:BOOL=<ON/OFF>`
* `-DSPDLOG_CONFIG_ONLY:BOOL=<ON/OFF>`
* `-DHDF5_DIR:PATH=<path to HDF5Config.cmake>` 
* `-DHDF5_ROOT:PATH=<path to HDF5 install-dir>` 
* `-DCONAN_PREFIX:PATH=<path to conan>`

## Linking

### Link using CMake targets (easy)
`h5pp` is easily imported into your project using CMake's `find_package`. Just point it to the `h5pp` install directory.
When found, targets are made available to compile and link to dependencies correctly.
A minimal `CMakeLists.txt` to use `h5pp` would look like:


```cmake
    cmake_minimum_required(VERSION 3.10)
    project(myProject)
    add_executable(myExecutable main.cpp)
    find_package(h5pp PATHS <path-to-h5pp-install-dir> REQUIRED) # If h5pp is installed through conda the path may be $ENV{CONDA_PREFIX}
    target_link_libraries(myExecutable PRIVATE h5pp::h5pp)

```
#### Targets explained

*  `h5pp::h5pp` is the main target including "everything" and should normally be the only target that you need -- headers,flags and (if enabled) the found/downloaded dependencies.
*  `h5pp::headers` links the `h5pp` headers only.
*  `h5pp::deps` has targets to link all the dependencies that were found/downloaded when `h5pp` was built. If you used `DOWNLOAD_METHOD=native` these targets are `Eigen3::Eigen`, `spdlog::spdlog` and `hdf5::hdf5`, which can of course be used independently.
*   If you used `DOWNLOAD_METHOD=conan` these targets are `CONAN_PKG::Eigen3`, `CONAN_PKG::spdlog` and `CONAN_PKG::HDF5`. If you used `DOWNLOAD_METHOD=none` this target is empty.
*  `h5pp::flags` sets compile and linker flags to  enable C++17 and std::filesystem library, i.e. `-std=c++17` and `-lstdc++fs`.


### Link manually (not as easy)
From the command-line you can of course link using linker flags such as `-std=c++17 -lstdc++fs -leigen3 -lspdlog -lhdf5_hl -lhdf5` provided these flags make sense on your system.
You could also use CMake's `find_package(...)` mechanism. A minimal `CMakeLists.txt` could be:

```cmake
    cmake_minimum_required(VERSION 3.10)
    project(myProject)
    
    add_executable(myExecutable main.cpp)
    target_include_directories(myExecutable PRIVATE <path-to-h5pp-headers>)
    # Setup h5pp
    target_compile_options(myExecutable PRIVATE cxx_std_17 )
    target_link_libraries(myExecutable PRIVATE  stdc++fs)
    
    # Possibly use find_package() here

    # Link dependencies (this is the tricky part)
    target_include_directories(myExecutable PRIVATE <path-to-Eigen3-include-dir>) 
    target_include_directories(myExecutable PRIVATE <path-to-spdlog-include-dir>) 
    target_include_directories(myExecutable PRIVATE <path-to-hdf5-include-dir>) 
    # Link dependencies (this is the difficult part). Note that you only need the C libs for HDF5.
    target_link_libraries(myExecutable PRIVATE hdf5_hl hdf5 rt dl m z pthread) # Possibly more libs, such as aec.

```

The difficult part is linking to HDF5 libraries and its dependencies.
When installing `h5pp` this is handled with a custom module for finding HDF5, defined in `cmake/FindHDF5.cmake`. This finds HDF5 installed
somewhere on your system (e.g. installed via `conda`,`apt`, `Easybuild`,etc) and defines a CMake target `hdf5::hdf5` with everything you need to link correctly.
You can use it too! Add the path pointing to `FindHDF5.cmake` to the variable `CMAKE_MODULE_PATH` from within your own project, e.g.:

```cmake
    list(APPEND CMAKE_MODULE_PATH path/to/bundled/FindHDF5.cmake)
    find_package(HDF5 1.10 COMPONENTS C HL REQUIRED)
    if(TARGET hdf5::hdf5)
            target_link_libraries(myExecutable PRIVATE hdf5::hdf5)
    endif()
```
