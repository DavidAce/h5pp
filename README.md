[![Build Status](https://travis-ci.org/DavidAce/h5pp.svg?branch=master)](https://travis-ci.org/DavidAce/h5pp)
[![Build Status](https://github.com/DavidAce/h5pp/workflows/C%2FC++%20CI/badge.svg)](https://github.com/DavidAce/h5pp/actions)
[![Anaconda-Server Badge](https://anaconda.org/davidace/h5pp/badges/installer/conda.svg)](https://conda.anaconda.org/davidace)
[ ![Download](https://api.bintray.com/packages/davidace/conan-public/h5pp%3Adavidace/images/download.svg) ](https://bintray.com/davidace/conan-public/h5pp%3Adavidace/_latestVersion)
# h5pp
`h5pp` is a C++17 wrapper for HDF5 with focus on simplicity.

In just a few lines of code, `h5pp` lets users read and write to disk in binary format. It supports complex data types in possibly multidimensional containers that are common in scientific computing.
In particular, `h5pp` makes it easy to store [**Eigen**](http://eigen.tuxfamily.org) matrices and tensors.

[Latest release](https://github.com/DavidAce/h5pp/releases) 


## Table of Contents

*   [Features](#features)
*   [Usage](#usage)
    *   [Example 1: Writing std::vector](#example-1-writing-stdvector)
    *   [Example 2: Reading std::vector](#example-2-reading-stdvector)
    *   [Example 3: Write and read an Eigen::Matrix](#example-3-write-and-read-an-eigenmatrix)
    *   [Example 4: Metadata in attributes](#example-4-metadata-in-attributes)
    *   [Debug and logging](#debug-and-logging)
    *   [File permissions](#file-permissions)
    *   [Extendable and non-extendable datasets](#extendable-and-non-extendable-datasets)
    *   [Compression](#compression)
    *   [Load data into Python](#load-data-into-python)
*   [Download](#download)
*   [Requirements](#requirements)
*   [Build and Install](#build-and-install)
    * [Option 1: Copy the headers](#option-1-copy-the-headers)
    * [Option 2: Build and install with CMake](#option-2-build-and-install-with-cmake)
    * [Opt-in automatic dependency installation](#opt-in-automatic-dependency-installation)
*   [Linking](#linking)


## Features
* Header-only C++17 template library
* Support for common data types:
    - `int`, `float`, `double` in unsigned and long versions
        - any of the above in C-style arrays
        - any of the above in `std::complex<>` form
        - any of the above in POD-structs with x,y or x,y,z data members. In `h5pp` these go by the name `Scalar2` and `Scalar3`.
            These work well together with `double2` or `float3` types found in CUDA
    - `std::string` and `char` arrays
    - Contiguous containers of types above, such as `std::vector`, with `.data()` methods
    - `Eigen` types such as `Matrix`, `Array` and `Tensor`, with automatic conversion to/from row major storage layout
    - Any multi-dimensional container with access to a C-style contiguous buffer (without conversion to/from row major)
* Modern CMake build, install and linking using targets
* (Opt-in) Automatically find or download dependencies using either [conan package manager](https://conan.io/) or native "CMake-only" methods


## Usage
Using `h5pp` is intended to be simple. After initializing a file, most of the work can be achieved using just two member functions `.writeDataset(...)` and `.readDataset(...)`.
To understand the basic usage, let's go through some examples.

### Example 1: Writing std::vector
To write data to file simply pass any supported object and a dataset name to `writeDataset`.
This example shows how to do this with a vector of doubles.

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


### Example 2: Reading std::vector
Reading from file works similarly with `readDataset`, with the only exception that you need to provide a container of the correct type.

```c++
    #include <h5pp/h5pp.h>
    
    int main() {
        
        // Initialize a file
        h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READONLY, h5pp::CreateMode::OPEN );
    
        // Initialize an empty a vector of doubles
        std::vector<double> v;
    
        // Read data. The vector is resized automatically by h5pp.
        file.readDataset(v, "myStdVector");
    
        return 0;
    }
```

**Notes** 
* this time we make use of file permissions in the constructor of `h5pp::File`, read more under [File permissions](#file-permissions).
* `h5pp` resizes `std` containers automatically. Resizing of C-style arrays is left to the user.
*  It is possible to query a dataset's *size* in advance, but there is no support (yet?) for querying its *type*. 


### Example 3: Write and read an Eigen::Matrix

```c++
    #include <h5pp/h5pp.h>
    
    int main() {
    
        // Initialize a file
        h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE);
    
        // Initialize a 10x10 Eigen matrix with random complex entries
        Eigen::MatrixXcd m1 = Eigen::MatrixXcd::Random(10, 10);
        
        // Write the matrix 
        // Inside the file, the data will be stored in a dataset named "myEigenMatrix" under the group "myMatrixCollection"
        file.writeDataset(m1, "myMatrixCollection/myEigenMatrix");
    
    
        // Read it back in one line. Note that we pass the type as a template parameter
        auto m2 = file.readDataset<Eigen::MatrixXcd> ("myMatrixCollection/myEigenMatrix");
    
        return 0;
    }
```

**Notes** 
* Once again we make use of file permissions in the constructor of `h5pp::File`, read more under [File permissions](#file-permissions).
* `h5pp` resizes `Eigen` containers automatically. Resizing of C-style arrays is left to the user.
* This time we put the dataset `myEigenMatrix` inside of the HDF5 group `myMatrixCollection`, which is automatically created by `h5pp`.
* We can use an alternative syntax to read datasets by assignment in one line.


### Example 4: Metadata in attributes
Metadata for a datasets or groups is stored in so-called "attributes". An attribute can be of any type, just like a dataset.
In fact, an attribute is very similar to a dataset, with the main difference being that it is supposed to be small and stored in the metadata headers of groups or datasets. 
Writing attributes works similarly with the function `writeAttribute(someObject,attributeName,targetLink)`.

```c++
    #include <h5pp/h5pp.h>
    #include <iostream>
    int main() {
    
        // Initialize a file
        h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::OPEN);
        // Write an integer to file
        file.writeDataset(42, "intGroup/myInt");
        // We can now write metadata, or "attributes" to the int.
        file.writeAttribute("this is some info about my int", "myInt_stringAttribute", "intGroup/myInt");
        file.writeAttribute(3.14, "myInt_doubleAttribute", "intGroup/myInt");
    
        // List all attributes associated with our int.
        // The following will print:
        //      myInt_stringAttribute
        //      myInt_doubleAttribute
        auto allAttributes = file.getAttributeNames("intGroup/myInt");
        for(auto & attr : allAttributes)std::cout << attr << std::endl; 
    
        // Read the attribute data back
        auto stringAttribute = file.readAttribute<std::string> ("myInt_stringAttribute", "intGroup/myInt");
        auto doubleAttribute = file.readAttribute<double>      ("myInt_doubleAttribute", "intGroup/myInt");
    
        return 0;
    }
```

**Notes** 
* Attributes can be written to groups or datasets. 
* A single dataset or group can have multiple attributes of different types. 


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



### File permissions
To define permissions use the settings `AccessMode::<mode>` and/or `CreateMode::<mode>` as arguments when initializing the file.
The possible modes are
* `AccessMode::`
    - `READONLY`  Read permission to the file.
    - `READWRITE` **(default)** Read and write permission to the file.
* `CreateMode::`
    - `OPEN` Open the file with the given name. Throws an error when file does not exist.
    - `RENAME` **(default)** File is created with an available file name. If `myFile.h5` already exists, `myFile-1.h5` is created instead. The appended integer is increased until an available name is found
    - `TRUNCATE` File is created with the given name and erases any pre-existing file. 

The defaults are chosen to avoid loss of data.
To give a concrete example, the syntax works as follows

```c++
    h5pp::File file("myDir/someFile.h5", h5pp::AccessMode::READWRITE, h5pp::CreateMode::TRUNCATE);
```

### Extendable and non-extendable datasets
By default, datasets in h5pp are created as non-extendable. This means that a dataset has a fixed size and can only be overwritten if the new data has the same size and shape.
In contrast, extendable datasets have dynamic size and can be overwritten by a larger dataset. Keep in mind that overwriting with a smaller dataset does not shrink the file size.

To swap the default behavior, use one of the methods below
```c++
    file.enableDefaultExtendable();
    file.disableDefaultExtendable();
```

You can also optionally pass a true/false argument when writing a new dataset to explicitly create it as extendable or non-extendable

```c++
    file.writeDataset(testvector, "testvector", true);      // Creates an extendable dataset
    file.writeDataset(testvector, "testvector", false);     // Creates a non-extendable dataset    
```

**Technical details:** 
- Extendability only applies for datasets with one or more dimensions. Zero-dimensional or "scalar" datasets are always as non-extendable (as `H5S_SCALAR`).
- Extendable datasets are "chunked" (as in `H5D_CHUNKED`), which means they can be read into memory in smaller chunks. This makes sense for large enough datasets.
- A dataset with one or more dimensions is **non-extendable by default, unless it is very large**.
- A non-extendable dataset smaller than 32 KB will be created as `H5D_COMPACT`, meaning it can fit in the metadata header.
- A non-extendable dataset between 32 KB and 512 KB will be created as `H5D_CONTIGUOUS`, meaning it is not "chunked" and can be read entirely at once.
- A non-extendable dataset larger than 512 KB will be **made into an extendable dataset** unless explicitly specified, because it makes sense to read large datasets in chunks.


### Compression
Extendable (or chunked) datasets can also be compressed with GZIP, if HDF5 was compiled with zlib support. Use these
functions to set or check the compression level:

```c++
    file.setCompressionLevel(9);            // 0 to 9: 0 to disable compression, 9 is maximum compression.
    file.getCompressionLevel();             // Gets the current compression level
    h5pp::checkIfCompressionIsAvailable();  // True if your installation of HDF5 has zlib support 
```


### Load data into Python
HDF5 data is easy to load into Python. Loading integer and floating point data is straightforward. Complex data is almost as simple.
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
- `git clone https://github.com/DavidAce/h5pp.git` and install (see below)
- (Debian only) Download the the [latest release](https://github.com/DavidAce/h5pp/releases) and install with apt: `sudo apt install ./h5pp_<version>_amd64.deb` 
- From conda: `conda install -c davidace h5pp`
- From [conan bintray repo](https://bintray.com/davidace/conan-public/h5pp%3Adavidace)


## Requirements
* C++17 capable compiler (tested with GCC version >= 7 and Clang version >= 7.0)
* CMake (tested with version >= 3.10)
* Dependencies:
    - [**HDF5**](https://support.hdfgroup.org/HDF5/) (tested with version >= 1.8).
    - [**Eigen**](http://eigen.tuxfamily.org) (tested with version >= 3.3.4).
    - [**spdlog**](https://github.com/gabime/spdlog) (tested with version >= 1.3.1)


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

By default, installing `h5pp` will install its headers under `CMAKE_INSTALL_PREFIX` and generate files
such as `h5ppConfig.cmake` `h5ppConfig.cmake` which lets the you, the user, consume `h5pp` using `find_package(h5pp)` in your own projects. Doing so will
define the target `h5pp::h5pp` which includes the headers and sets necessary compiler flags (e.g. -std=c++17). 
If not given, `CMAKE_INSTALL_PREFIX` defaults to `${CMAKE_BINARY_DIR}/install`, where `${CMAKE_BINARY_DIR}` is the directory you are building from. Building 
the.




#### Opt-in automatic dependency installation
The CMake flag `DOWNLOAD_METHOD` controls the automated behavior for finding or installing dependencies. It can take one of three valid strings:
* `none` (default) all handling of dependencies is disabled and linking is left to the user.
* `conan` to install dependencies using the [conan package manager](https://conan.io/). This method is guided by `conanfile.txt` found in this project's root directory.
    This method requires conan to be installed prior (for instance through `pip`, `conda`, `apt`, etc). To let CMake find conan you have three options:
  - Add conan install (or bin) directory to the environment variable `PATH`.
  - Export conan install (or bin) directory in the environment variable `CONAN_PREFIX`, i.e. from command line: `export CONAN_PREFIX=<path-to-conan>` 
  - Give the variable `CONAN_PREFIX` directly to CMake, i.e. from command line: `cmake -DCONAN_PREFIX:PATH=<path-to-conan> ...`
* `native` to find dependencies pre-installed somewhere on your system using `find_package`. If finding fails, dependencies are downloaded and built from source during CMake configure, 
    and installed under `CMAKE_INSTALL_PREFIX`. There are several variables you can pass to CMake to guide `find_package` calls, see [build options](#cmake-build-options). 
    By default it searches standard system install directories as well as typical anaconda3 or miniconda directories.


#### CMake build options

The `cmake` step above takes several options, `cmake [-DOPTIONS=var] ../ `:
* `-DCMAKE_INSTALL_PREFIX:PATH=<install-dir>` to specify install directory (default: `${CMAKE_BINARY_DIR}/install`).
* `-DBUILD_SHARED_LIBS:BOOL=<ON/OFF>` to link dependencies with static or shared libraries (default: `OFF`)
* `-DCMAKE_BUILD_TYPE=Release/Debug` to specify build type of tests and examples (default: `Release`)
* `-DENABLE_TESTS:BOOL=<ON/OFF>` to run ctests after build (recommended!) (default: `OFF`).
* `-DBUILD_EXAMPLES:BOOL=<ON/OFF>` to build example programs (default: `OFF`)
* `-DDOWNLOAD_METHOD=<none/conan/native>` to select download method. (default: `none`).
* `-DH5PP_PRINT_INFO:BOOL=<ON/OFF>` to print extra CMake info about the host and generated targets during configure (default: `OFF`).
* `-DAPPEND_LIBSUFFIX:BOOL=<ON/OFF>` Append a directory with the library name to install directory, i.e. `CMAKE_INSTALL_PREFIX/<libname>/`. This
    is useful when you want to install `h5pp`, `hdf5`, `Eigen3` and `spdlog` in separate folders (default: `OFF`).
* `-DPREFER_CONDA_LIBS:BOOL=<ON/OFF>` to prioritize finding dependencies  `hdf5`, `Eigen3` and `spdlog` installed through conda (default: `OFF`).
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

-  `h5pp::h5pp` is the main target including "everything" and should normally be the only target that you need -- headers,flags and (if enabled) the found/downloaded dependencies.
-  `h5pp::headers` links the `h5pp` headers only.
-  `h5pp::deps` has targets to link all the dependencies that were found/downloaded when `h5pp` was built. If you used `DOWNLOAD_METHOD=native` these targets are `Eigen3::Eigen`, `spdlog::spdlog` and `hdf5::hdf5`, which can of course be used independently.
    If you used `DOWNLOAD_METHOD=conan` these targets are `CONAN_PKG::Eigen3`, `CONAN_PKG::spdlog` and `CONAN_PKG::HDF5`. If you used `DOWNLOAD_METHOD=none` this target is empty.
-  `h5pp::flags` sets compile flags that you need to compile with `h5pp`. These flags enable C++17 and filesystem headers, i.e. `-std=c++17` and `-lstdc++fs`.


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
When installing `h5pp` this is handled with a helper function defined in `cmake/FindPackageHDF5.cmake` which finds HDF5 installed
somewhere on your system (e.g. installed via `conda`,`apt`, `Easybuild`,etc) and defines a CMake target `hdf5::hdf5` with everything you need to link correctly.
You can use it too! If you copy `cmake/FindPackageHDF5.cmake` to your project, find HDF5 by including it and using the function:

```cmake
    include(FindPackageHDF5.cmake)
    find_package_hdf5()
    if(TARGET hdf5::hdf5)
            target_link_libraries(myExecutable PRIVATE hdf5::hdf5)
    endif()
```
