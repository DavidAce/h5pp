[![Ubuntu 24.04](https://github.com/DavidAce/h5pp/actions/workflows/ubuntu24.yml/badge.svg?branch=v2)](https://github.com/DavidAce/h5pp/actions/workflows/ubuntu24.yml)
[![Ubuntu 26.04](https://github.com/DavidAce/h5pp/actions/workflows/ubuntu26.yml/badge.svg?branch=v2)](https://github.com/DavidAce/h5pp/actions/workflows/ubuntu26.yml)
[![Windows 2025](https://github.com/DavidAce/h5pp/actions/workflows/windows2025.yml/badge.svg?branch=v2)](https://github.com/DavidAce/h5pp/actions/workflows/windows2025.yml)
[![macOS 26](https://github.com/DavidAce/h5pp/actions/workflows/macos26.yml/badge.svg?branch=v2)](https://github.com/DavidAce/h5pp/actions/workflows/macos26.yml)
[![Documentation Status](https://readthedocs.org/projects/h5pp/badge/?version=latest)](https://h5pp.readthedocs.io/en/latest/?badge=latest)
[![Conan](https://img.shields.io/badge/Install%20with-conan-green)](https://conan.io/center/h5pp)
[![codecov](https://codecov.io/gh/davidace/h5pp/branch/dev/graph/badge.svg)](https://codecov.io/gh/davidace/h5pp)
---

# h5pp

`h5pp` is a high-level C++20 interface for the [HDF5](https://www.hdfgroup.org/) C library. With simplicity in
mind, `h5pp` lets users store common C++ data types into portable binary [HDF5](https://www.hdfgroup.org/) files.

[Latest release](https://github.com/DavidAce/h5pp/releases)

[Documentation](https://h5pp.readthedocs.io)

Go to [examples](https://github.com/DavidAce/h5pp/tree/master/examples) to learn how to use `h5pp`.

Go to [quickstart](https://github.com/DavidAce/h5pp/tree/v2/quickstart) to see ways of installing `h5pp`.

---

## Table of Contents

* [Introduction](#introduction)
* [Features](#features)
* [Examples](#examples)
* [Migrating from v1](#migrating-from-v1)
* [Get h5pp](#get-h5pp)
* [Requirements](#requirements)
* [Install](#install)

## Introduction

[HDF5](https://www.hdfgroup.org/) is a portable file format for storing large datasets efficiently. HDF5 has
official [low-level API's for C and Fortran](https://portal.hdfgroup.org/display/HDF5/Core+Library) with wrappers
for C++ and Java, and third-party bindings for Python, Julia, Matlab and many other languages. This makes HDF5 a
great tool for handling data in a collaborative setting.

Although well documented, the low-level C API is vast and using it directly can be challenging. There are many
high-level wrappers already that help the user experience, but as a matter of opinion, things could be even simpler.

### Goals

`h5pp` is a high-level C++20 interface for the HDF5 C library which aims to be simple to use:

* Read and write common C++ types in a single line of code.
* Meaningful logs and error messages.
* No prior knowledge of HDF5 is required.
* Simple access to HDF5 features like tables, compression, chunking and hyperslabs.
* Simple installation with opt-in automatic installation of dependencies.
* Simple documentation.

## Features

* Header-only C++20 library.
* High-level front-end to the C API of the HDF5 library.
* Two complementary public API styles:
    * A handle API such as `file.dataset("path").write(data)` and `file.attribute("path", "name").read<T>()`.
    * A file-level API such as `file.writeDataset(data, "path")` and `file.readAttribute<T>("path", "name")`.
* Type support:
    * all numeric types: `(u)int#_t`, `float`, `double`, `long double`.
    * **`std::complex<>`** with any of the types above.
    * CUDA-style POD-structs with `x,y` or `x,y,z` members as atomic type, such as `float3` or `double2`. These work
      with any of the types above. In `h5pp` these go by the name `Scalar2<>` and `Scalar3<>`.
    * Contiguous containers with a `.data()` member, such as `std::vector<>`.
    * Raw C-style arrays or pointer to buffer + dimensions.
    * [**Eigen**](http://eigen.tuxfamily.org) types such as `Eigen::Matrix<>`, `Eigen::Array<>` and `Eigen::Tensor<>`,
      with automatic conversion to/from row-major storage
    * Text types `std::string`, `char` arrays, and `std::vector<std::string>`.
    * Structs as HDF5 Compound types ([example](https://github.com/DavidAce/h5pp/blob/master/examples/example-04a-compound-datatype-scalars.cpp))
    * Structs as HDF5 Tables (with user-defined compound HDF5 types for entries)
    * Ragged "variable-length" data in HDF5 Table columns using `h5pp::varr_t<>` and `h5pp::vstr_t`.
* Modern CMake installation of `h5pp` and (opt-in) installation of dependencies.
* Multi-platform: Linux, Windows, OSX. (Developed under Linux).

## Examples

### Write an `std::vector`

```c++
    #include <h5pp/h5pp.h>
    int main() {
        std::vector<double> v = {1.0, 2.0, 3.0};      // Define some data
        h5pp::File file("somePath/someFile.h5");      // Create or open a file

        file.dataset("myStdVector").write(v);         // Handle API
        // file.writeDataset(v, "myStdVector");       // Equivalent file-level API
    }
```

### Read an `std::vector`

```c++
    #include <h5pp/h5pp.h>
    int main() {
        h5pp::File file("somePath/someFile.h5", h5pp::FileAccess::READONLY);      // Open an existing file

        auto v = file.dataset("myStdVector").read<std::vector<double>>();         // Handle API
        // auto v = file.readDataset<std::vector<double>>("myStdVector");         // Equivalent file-level API
    }
```

Find more code examples in the [examples directory](https://github.com/DavidAce/h5pp/tree/master/examples).

## Migrating from v1

For `h5pp` 2.x, `#include <h5pp/h5pp.h>` gives the new v2 API by default.

If you want to keep using the legacy API while migrating, include:

```c++
    #include <h5pp/v1/h5pp.h>
```

The most common dataset and attribute calls still work in the file-level API:

```c++
    file.writeDataset(data, "group/dataset");
    auto data2 = file.readDataset<std::vector<double>>("group/dataset");

    file.writeAttribute("group/dataset", "unit", std::string("m/s"));
    auto unit = file.readAttribute<std::string>("group/dataset", "unit");
```

The canonical v2 style is the handle API:

```c++
    file.dataset("group/dataset").write(data);
    auto data2 = file.dataset("group/dataset").read<std::vector<double>>();

    file.attribute("group/dataset", "unit").write(std::string("m/s"));
    auto unit = file.attribute("group/dataset", "unit").read<std::string>();
```

Advanced file-level wrappers remain available during migration, but many of them are deprecated in v2 and emit compiler warnings with the recommended handle-based replacement.


## Get h5pp

There are currently 2 ways to obtain `h5pp`:

* From [conan-center](https://conan.io/center/h5pp).
* From [GitHub](https://github.com/DavidAce/h5pp).

## Requirements

* C++20 capable compiler
* CMake version >= 3.24 (>= 3.26 for the bundled CMake dependency provider)
* [**HDF5**](https://support.hdfgroup.org/HDF5/)  library, version >= 1.8

### Optional dependencies

* [**Eigen**](http://eigen.tuxfamily.org) >= 3.4.0: Store Eigen containers. Enable with `#define H5PP_USE_EIGEN3`.
* [**spdlog**](https://github.com/gabime/spdlog) >= 1.12.0: Logging library. Enable with `#define H5PP_USE_SPDLOG`.
* [**fmt**](https://github.com/fmtlib/fmt) >= 9.1.0: String formatting (used in `spdlog`). Enable with `#define H5PP_USE_FMT`.

**NOTE:** Logging works the same with or without [Spdlog](https://github.com/gabime/spdlog) enabled. When Spdlog is *
not* found, a hand-crafted logger is used in its place to give identical output but without any performance
considerations (implemented with STL lists, strings and streams).

The providers select newer releases than these minimums. In particular, the
CMake and Conan providers select Eigen 5.0.1; an existing Eigen 3.4.0 installation
can be used without either provider. The CMake provider builds HDF5 2.1.1;
the Conan recipe currently selects HDF5 1.x. See the
[installation guide](docs/source/installation.md#requirements) for the full version list.

## Install

This branch describes h5pp **2.0.0**. Use the `v2` checkout; do not assume the
same version has already been published to ConanCenter.

```bash
git clone --branch v2 https://github.com/DavidAce/h5pp.git
cd h5pp
```

### Use a dependency-provider preset

With CMake 3.26 or newer, the bundled CMake provider downloads and builds dependencies:

```bash
cmake --preset release-cmake
cmake --build --preset release-cmake --parallel 6
cmake --install build/release-cmake
```

This preset installs under `install/release-cmake`. Alternatively, use
`release-conan` with CMake >= 3.24 and Conan >= 2.0.5 on `PATH`; it installs h5pp
under `install/release-conan` and uses Conan to resolve dependencies.
Both presets enable optional integrations, tests, and examples.

### Use dependencies you installed yourself

Without a provider, CMake finds existing dependencies:

```bash
cmake -S . -B build/local \
    -DCMAKE_INSTALL_PREFIX="$PWD/install/local" \
    -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
    -DH5PP_ENABLE_EIGEN3=ON -DH5PP_ENABLE_SPDLOG=ON
cmake --build build/local --parallel 6
cmake --install build/local
```

Use `CMAKE_PREFIX_PATH` to point to dependencies installed outside standard locations.
The old `H5PP_PACKAGE_MANAGER` settings have been replaced by dependency providers.
For a local Conan package, run `conan create . --build=missing -s:h compiler.cppstd=20`.

### Copy the headers

`h5pp` is header-only. Copy the files under `include` to your project and then add `#include <h5pp/h5pp.h>`.

Read more about linking h5pp to its dependencies [here](docs/source/installation.md#link-from-a-cmake-project)
