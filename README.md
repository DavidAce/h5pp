[![Ubuntu 16.04](https://github.com/DavidAce/h5pp/workflows/Ubuntu%2016.04/badge.svg?branch=master)](https://github.com/DavidAce/h5pp/actions)
[![Ubuntu 20.04](https://github.com/DavidAce/h5pp/workflows/Ubuntu%2020.04/badge.svg?branch=master)](https://github.com/DavidAce/h5pp/actions)
[![Windows 10](https://github.com/DavidAce/h5pp/workflows/Windows%2010/badge.svg?branch=master)](https://github.com/DavidAce/h5pp/actions)
[![MacOS 10.15](https://github.com/DavidAce/h5pp/workflows/macOS%2010.15/badge.svg?branch=master)](https://github.com/DavidAce/h5pp/actions)
[![Documentation Status](https://readthedocs.org/projects/h5pp/badge/?version=latest)](https://h5pp.readthedocs.io/en/latest/?badge=latest)
[![Conan](https://img.shields.io/badge/Install%20with-conan-green)](https://conan.io/center/h5pp)
[![OS](https://img.shields.io/badge/OS-Linux%7COSX%7CWindows-blue)](https://img.shields.io/badge/OS-Linux%7COSX%7CWindows-blue)
[![codecov](https://codecov.io/gh/davidace/h5pp/branch/dev/graph/badge.svg)](https://codecov.io/gh/davidace/h5pp)
---

# h5pp

`h5pp` is a high-level C++17 interface for the [HDF5](https://www.hdfgroup.org/) C library. With simplicity in mind, `h5pp` lets users store common C++ data types into portable
binary [HDF5](https://www.hdfgroup.org/) files.

[Latest release](https://github.com/DavidAce/h5pp/releases)

[Documentation](https://h5pp.readthedocs.io)

Go to [quickstart](https://github.com/DavidAce/h5pp/tree/master/quickstart) to see install examples

Go to [examples](https://github.com/DavidAce/h5pp/tree/master/examples) to learn how to use `h5pp`

---

## Table of Contents

* [Introduction](#introduction)
* [Features](#features)
* [Usage](#usage)
    * [Example: Writing an std::vector](#example-write-an-stdvector)
    * [Example: Read an std::vector](#example-read-an-stdvector)
    * [Debug and logging](#debug-and-logging)
    * [File Access](#file-access)
    * [Storage layout](#storage-layout)
    * [Compression](#compression)
    * [Tips](#tips)
        * [View data](#view-data)
        * [Load data into Python](#load-data-into-python)
* [Requirements](#requirements)
* [Get h5pp](#get-h5pp)
* [Install](#install)
    * [Option 1: Install with Conan](#option-1-install-with-conan-recommended)
    * [Option 2: Install with CMake](#option-2-install-with-cmake)
        * [Opt-in automatic dependency installation with CMake](#opt-in-automatic-dependency-installation-with-cmake)
        * [CMake options](#cmake-options)
    * [Option 3: Copy the headers](#option-3-copy-the-headers)
* [Link](#link)
    * [Link using CMake targets (easy)](#link-using-cmake-targets-easy)
    * [Link manually (not as easy)](#link-manually-not-as-easy)
* [Uninstall](#uninstall)
* [To-do](#to-do)

## Introduction

[HDF5](https://www.hdfgroup.org/) is a portable file format for storing large datasets efficiently. With
official [low-level API's for C and Fortran](https://portal.hdfgroup.org/display/HDF5/Core+Library), wrappers for C++
and Java and third-party bindings to Python, Julia, Matlab and many others, HDF5 is a great tool for manipulating data
in a collaborative setting.

Although well documented, the low-level C API is vast and using it directly can be challenging. There are many
high-level wrappers already that help the user experience, but as a matter of opinion, things could be even simpler.

`h5pp` is a high-level C++17 wrapper of the HDF5 C library which aims to be simple to use:

* Read and write common C++ types in a single line of code.
* No prior knowledge of HDF5 is required.
* Meaningful logs and error messages.
* Use HDF5 with modern, idiomatic, type-safe C++.
* Default settings let simple tasks stay simple, e.g., storage layout, chunking and compression.
* Advanced tasks remain possible, e.g. MPI parallelism.
* Simple installation with modular dependencies and opt-in automation.
* Simple documentation (work in progress)

## Features

* Header-only C++17 template library
* High-level front-end to the C API of HDF5
* Modern CMake installation of `h5pp` and its dependencies (opt-in)
* Multi-platform: Linux, Windows, OSX. (Developed under Linux)
* Supports:
    * all numeric types: `(u)int#_t`, `float`, `double`, `long double`
    * **`std::complex<>`** with any of the types above
    * CUDA-style POD-structs with `x,y` or `x,y,z` members as atomic type, such as `float3` or `double2`. These work
      with any of the types above. In `h5pp` these go by the name `Scalar2<>` and `Scalar3<>`.
    * Contiguous containers with a `.data()` member, such as `std::vector<>`
    * Text types `std::string`, `char` arrays, and `std::vector<std::string>`
    * C-style arrays or pointer to buffer
    * [**Eigen**](http://eigen.tuxfamily.org) types such as `Eigen::Matrix<>`, `Eigen::Array<>` and `Eigen::Tensor<>`,
      with automatic conversion to/from row-major storage
    * Structs as compound HDF5 types (
      see [example](https://github.com/DavidAce/h5pp/blob/master/examples/example-04a-custom-struct-easy.cpp))
    * Structs as HDF5 tables (with user-defined compound HDF5 types for entries)

## Usage

Using `h5pp` is intended to be simple. After initializing a file, most the work can be achieved using just two member
functions `.writeDataset(...)` and `.readDataset(...)`.

### Example: Write an `std::vector`

```c++
    #include <h5pp/h5pp.h>
    int main() {
        std::vector<double> v = {1.0, 2.0, 3.0};    // Define a vector
        h5pp::File file("somePath/someFile.h5");    // Create a file 
        file.writeDataset(v, "myStdVector");        // Write the vector into a new dataset "myStdVector"
    }
```

### Example: Read an `std::vector`

```c++
    #include <h5pp/h5pp.h>
    int main() {
        h5pp::File file("somePath/someFile.h5", h5pp::FileAccess::READWRITE);    // Open (or create) a file
        auto v = file.readDataset<std::vector<double>>("myStdVector");               // Read the dataset from file
    }
```

Find more code examples in the [examples directory](https://github.com/DavidAce/h5pp/tree/master/examples).

### File Access

`h5pp` offers more flags for file access permissions than HDF5. The new flags are primarily intended to prevent
accidental loss of data, but also to clarify intent and avoid mutually exclusive options.

The flags are listed in the order of increasing "danger" that they pose to previously existing files.

| Flag                 | File exists                         | No file exists  | Comment                                                                                           |
|----------------------|-------------------------------------|-----------------|---------------------------------------------------------------------------------------------------|
| `READONLY`           | Open with read-only access          | Throw error     | Never writes to disk, fails if the file is not found                                              |
| `COLLISION_FAIL`     | Throw error                         | Create new file | Never deletes existing files and fails if it already exists                                       |
| `RENAME` **default** | Create renamed file                 | Create new file | Never deletes existing files. Appends "-#" (#=1,2,3...) to the stem of existing filename          |
| `READWRITE`          | Open with read-write access         | Create new file | Never deletes existing files, but is allowed to open/modify                                       |
| `BACKUP`             | Rename existing file and create new | Create new file | Avoids collision by backing up the existing file, appending ".bak_#" (#=1,2,3...) to the filename |
| `REPLACE`            | Truncate (overwrite)                | Create new file | Deletes the existing file and creates a new one in place                                          |

* When a new file is created, the intermediate directories are always created automatically.
* When a new file is created, `READWRITE` access to it is implied.

To give a concrete example, the syntax works as follows

```c++
    h5pp::File file("somePath/someFile.h5", h5pp::FileAccess::REPLACE);
```

### Storage Layout

HDF5 offers three [storage layouts](https://support.hdfgroup.org/HDF5/Tutor/layout.html#lo-define):

* `H5D_COMPACT`:  For scalar or small datasets which can fit in the metadata header. Default on datasets smaller than 32
  KB.
* `H5D_CONTIGUOUS`: For medium size datasets. Default on datasets smaller than 512 KB.
* `H5D_CHUNKED`: For large datasets. Default on datasets larger than 512 KB. This layout has some additional features:
    * Chunking, portioning of the data to improve IO performance by caching more efficiently. Chunk dimensions are
      calculated by `h5pp` if not given by the user.
    * Compression, disabled by default, and only available if HDF5 was built with zlib enabled.
    * Resize datasets. Note that the file size never decreases, for instance after overwriting with a smaller dataset.

`h5pp` can automatically determine the storage layout for each new dataset. To specify the layout manually, pass it as a
third argument when writing a new dataset, for instance:

```c++
    file.writeDataset(myData, "science/myChunkedData", H5D_CHUNKED);      // Creates a chunked dataset
```

### Compression

Chunked datasets can be compressed if HDF5 was built with zlib support. Use these functions to set or check the
compression level:

```c++
    file.setCompressionLevel(3);            // 0 to 9: 0 to disable compression, 9 for maximum compression. Recommended 2 to 5
    file.getCompressionLevel();             // Gets the current compression level
    h5pp::hdf5::isCompressionAvaliable();   // True if your installation of HDF5 has zlib support 
```

or pass a temporary compression level as the fifth argument when writing a dataset:

```c++
    file.writeDataset(myData, "science/myCompressedData", H5D_CHUNKED, std::nullopt, 3); // Creates a chunked dataset with compression level 3.
```

or use the special member function for this task:

```c++
   file.writeDataset_compressed(myData, "science/myCompressedData", 3) // // Creates a chunked dataset with compression level 3 (default).
```

### Debug and logging

`h5pp` uses [spdlog](https://github.com/gabime/spdlog) to emits messages to stdout about its internal state during read/write operatios. 
There are 7 levels of verbosity:

* `0: trace` (highest)
* `1: debug`
* `2: info`  (default)
* `3: warn`
* `4: error`
* `5: critical` (lowest)
* `6: off`

Set the level when constructing a h5pp::File or by calling the function `.setLogLevel(...)`:

```c++
    // This way...
    h5pp::File file("myDir/someFile.h5", h5pp::FileAccess::REPLACE, h5pp::LogLevel::debug); 
    // or this way
    file.setLogLevel(h5pp::LogLevel::trace);                                                                       
```

**NOTE:** Logging works the same with or without [spdlog](https://github.com/gabime/spdlog) enabled. When spdlog is *
not* found, a hand-crafted logger is used in its place to give identical output but without any performance
considerations (implemented with STL lists, strings and streams).

### Tips

#### **NEW:** [h5du](https://github.com/DavidAce/h5du)

List the size of objects inside an HDF5 file with [h5du](https://github.com/DavidAce/h5du).

#### View data

Try [HDF Compass](https://support.hdfgroup.org/projects/compass)
or [HDFView](https://www.hdfgroup.org/downloads/hdfview). Both are available in Ubuntu's package repository.

#### Load data into Python

HDF5 data is easy to load into Python using [h5py](https://docs.h5py.org/en/stable). Loading integer and floating point
data is straightforward. Complex data is almost as simple, so let's use that as an example.

HDF5 does not support complex types natively, but `h5pp`enables this by using a custom compound HDF5 type with `real`
and `imag` fields. Here is a python example which uses [h5py](https://docs.h5py.org/en/stable) to load 1D arrays from an
HDF5 file generated with `h5pp`:

```python
    import h5py
    import numpy as np
    file  = h5py.File('myFile.h5', 'r')
    
    # previously written as std::vector<double> in h5pp
    myDoubleArray = file['double-array-dataset'][()]                                     
    
    # previously written as std::vector<std::complex<double>> in h5pp
    myComplexArray = file['complex-double-array-dataset'][()].view(dtype=np.complex128)
```

Notice the cast to `dtype=np.complex128` which interprets each element of the array as two `doubles`, i.e. the real and
imaginary parts are `2 * 64 = 128` bits.

## Get h5pp

There are currently 3 ways to obtain `h5pp`:

* From [conan-center](https://conan.io/center/h5pp/1.9.0)
* `git clone https://github.com/DavidAce/h5pp.git` and install (see below)
* (Ubuntu/Debian only) Download the [latest release](https://github.com/DavidAce/h5pp/releases) and install with
  apt: `sudo apt install ./h5pp_<version>_amd64.deb`

## Requirements

* C++17 capable compiler. GCC version >= 7 or Clang version >= 7.0
* CMake version >= 3.15
* [**HDF5**](https://support.hdfgroup.org/HDF5/)  library, version >= 1.8

### Optional dependencies

* [**Eigen**](http://eigen.tuxfamily.org): Write Eigen matrices and tensors directly. Tested with version >= 3.3.4
* [**spdlog**](https://github.com/gabime/spdlog): Enables logging for debug purposes. Tested with version >= 1.3.1
* [**fmt**](https://github.com/fmtlib/fmt): String formatting library (used in `spdlog`).

**NOTE:** Logging works the same with or without [Spdlog](https://github.com/gabime/spdlog) enabled. When Spdlog is *
not* found, a hand-crafted logger is used in its place to give identical output but without any performance
considerations (implemented with STL lists, strings and streams).

## Install

Read the instructions [here](https://h5pp.readthedocs.io/en/latest/installation.html#installation) or see installation examples under [quickstart](https://github.com/DavidAce/h5pp/tree/master/quickstart). 
Find a summary below.

### Option 1: Install with Conan (Recommended)

Install and configure [conan](https://conan.io), then run the following command to install from [conan center](https://conan.io/center/h5pp):

```
> conan install h5pp/1.9.0@ --build=missing
```

### Option 2: Install with CMake

Git clone and build from command line:

```bash
    git clone https://github.com/DavidAce/h5pp.git
    mkdir h5pp/build
    cd h5pp/build
    cmake -DCMAKE_INSTALL_PREFIX=<install-dir>  ../
    make
    make install
```

Read more about `h5pp` CMake options in the [documentation](https://h5pp.readthedocs.io/en/latest/installation.html)


### Option 3: Copy the headers

`h5pp` is header-only. Copy the files under `include` to your project and then add `#include <h5pp/h5pp.h>`.

Read more about linking h5pp to its dependencies [here](https://h5pp.readthedocs.io/en/latest/installation.html#link)

## To-do

* For version 2.0.0
    * Single header
    * Compiled-library mode

In no particular order

* Continue adding documentation
* Expand the pointer-to-data interface
* Expand testing using catch2 for more edge-cases in
    * filesystem permissions
    * user-defined types
    * tables
* Expose more of the C-API:
    * More support for parallel read/write with MPI

  
