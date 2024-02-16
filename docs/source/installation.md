# Installation

## Get h5pp

There are currently 3 ways to obtain `h5pp`:

* From [conan-center](https://conan.io/center/h5pp).
* From [GitHub](https://github.com/DavidAce/h5pp).
* As a `.deb` package from [latest release](https://github.com/DavidAce/h5pp/releases) (Ubuntu/Debian only).

## Requirements

* C++17 capable compiler. GCC version >= 7 or Clang version >= 7.0
* CMake version >= 3.15
* [**HDF5**](https://support.hdfgroup.org/HDF5/)  library, version >= 1.8

### Optional dependencies

* [**Eigen**](http://eigen.tuxfamily.org) >= 3.3.4: Store Eigen containers. Enable with `#define H5PP_USE_EIGEN3`.
* [**spdlog**](https://github.com/gabime/spdlog) >= 1.3.1: Logging library. Enable with `#define H5PP_USE_SPDLOG`.
* [**fmt**](https://github.com/fmtlib/fmt) >= 6.1.2: String formatting (used in `spdlog`). Enable with `#define H5PP_USE_FMT`.

**NOTE:** Logging works the same with or without [Spdlog](https://github.com/gabime/spdlog) enabled. When Spdlog is *
not* found, a hand-crafted logger is used in its place to give identical output but without any performance
considerations (implemented with STL lists, strings and streams).

## Install

See full installation examples under [quickstart](https://github.com/DavidAce/h5pp/tree/master/quickstart). Find a
summary below.

### Option 1: Install with Conan (Recommended)

Install and configure [conan](https://conan.io), then run the following command to install from [conan center](https://conan.io/center/h5pp):

```bash
> conan install h5pp/1.11.2
```

The flag `--build=missing` lets conan install dependencies: `HDF5`, `Eigen` and `fmt` and `spdlog`.

Note that you can also (as an alternative) use the file `conanfile.py` bundled with h5pp to create and install directly
after cloning this git repo

```bash
> git clone https://github.com/DavidAce/h5pp.git
> cd h5pp
> conan create . davidace/stable --build=missing
```

After installation, use `h5pp` like any other conan package. For more information refer to
the [conan docs](https://docs.conan.io/en/latest/getting_started.html) or have a look
at [quickstart](https://github.com/DavidAce/h5pp/tree/master/quickstart).

### Option 2: Install with CMake

After cloning this repository, build the library just as any CMake project. For example, run the following commands:

```bash
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=<install-dir>  <source-dir>
    make
    make install
```

Headers will be installed under `<install-dir>/include` and config files under `<install-dir>/share/h5pp/cmake`. These
config files allow you to use`find_package(h5pp)` in your own projects, which in turn defines the target `h5pp::h5pp`
with everything you need to link `h5pp` correctly (including dependencies if you so choose: see below).

#### CMake Presets

Git clone and use one of the bundled CMake Presets to configure and build the project.
In this case we choose `release-cmake` to install all the dependencies using just CMake. 

```bash
    git clone https://github.com/DavidAce/h5pp.git
    cd h5pp
    cmake --preset=release-cmake         # Configure. Optionally add -DCMAKE_INSTALL_PREFIX=<install-dir>
    cmake --build --preset=release-cmake # Builds tests and examples. Optionally add --parallel=<num cores>
    cmake --install build/release-cmake  # Install to <install-dir> (default is ./install)
    ctest --preset=release-cmake         # Optionally run tests
```


#### Opt-in automatic dependency installation with CMake

The CMake flag `H5PP_PACKAGE_MANAGER` controls the automated behavior for finding or installing dependencies. It can
take one of these string values:

| Value                | Description                                                                                                                                          |
|----------------------|------------------------------------------------------------------------------------------------------------------------------------------------------|
| `none`               | Skip handling dependencies                                                                                                                           |
| `find` **(default)** | Use CMake's `find_package` to find dependencies                                                                                                     |
| `cmake` **¹**        | Use isolated CMake instances to download and install dependencies during configure. Disregards pre-installed dependencies on your system             |
| `fetch` **²**        | Use FetchContent to download and install dependencies. Disregards pre-installed dependencies on your system                                          |
| `cpm` **³**          | Use [CPM](https://github.com/cpm-cmake/CPM.cmake)to download and install dependencies. Disregards pre-installed dependencies on your system          |
| `find-or-cmake`      | Start with `find` and then go to `cmake` if not found                                                                                                |
| `find-or-fetch`      | Start with `find` and then go to `fetch` if not found                                                                                                |
| `find-or-cpm`        | Start with `find` and then go to `cpm` if not found                                                                                                  |
| `conan` **⁴**        | Use the [Conan package manager](https://conan.io/) to download and install dependencies automatically. Disregards libraries elsewhere on your system |

There are several variables you can pass to CMake to guide `find_package` calls and install location,
see [CMake options](#cmake-options) below.

**¹** Dependencies are installed into `${H5PP_DEPS_INSTALL_DIR}[/<PackageName>]`, where `H5PP_DEPS_INSTALL_DIR` defaults
to `CMAKE_INSTALL_PREFIX` and optionally `/<PackageName>` is added if `H5PP_PREFIX_ADD_PKGNAME=TRUE`

**²** Dependencies are installed into `${CMAKE_INSTALL_PREFIX}[/<PackageName>]`.

**³** Dependencies are installed into `${CMAKE_INSTALL_PREFIX}`.

**⁴** Conan is guided by `conanfile.txt` found in this project's root directory. This method requires conan to be
installed prior (for instance through `pip`, `conda`, `apt`, etc). To let CMake find conan you have three options:

* Add Conan install (or bin) directory to the environment variable `PATH`.
* Export Conan install (or bin) directory in the environment variable `CONAN_PREFIX`, i.e. from command
  line: `export CONAN_PREFIX=<path-to-conan>`
* Give the variable `CONAN_PREFIX` directly to CMake, i.e. from command
  line: `cmake -DCONAN_PREFIX:PATH=<path-to-conan> ...`

#### CMake options

The `cmake` step above takes several options, `cmake [-DOPTIONS=var] ../ `:

| Option                    | Default                | Description                                                                                                                            |
|---------------------------|------------------------|----------------------------------------------------------------------------------------------------------------------------------------|
| `H5PP_ENABLE_EIGEN3`      | `OFF`                  | Enables `Eigen` linear algebra library support                                                                                         |
| `H5PP_ENABLE_FMT`         | `OFF`                  | Enables `{fmt}` string formatting library                                                                                              |
| `H5PP_ENABLE_SPDLOG`      | `OFF`                  | Enables `spdlog` support for logging `h5pp` internal info to stdout (implies fmt)                                                      |
| `H5PP_PACKAGE_MANAGER`    | `find`                 | Download method for dependencies, select, `find`, `cmake`,`fetch`, `cpm`, `find-or-cmake`, `find-or-fetch` or `conan`                  |
| `BUILD_SHARED_LIBS`       | `OFF`                  | Link dependencies with static or shared libraries                                                                                      |
| `CMAKE_INSTALL_PREFIX`    | None                   | Install directory for `h5pp` and dependencies                                                                                          |
| `H5PP_DEPS_INSTALL_DIR`   | `CMAKE_INSTALL_PREFIX` | Install directory for dependencies only (if a different one is desired)                                                                |
| `H5PP_PREFIX_ADD_PKGNAME` | `OFF`                  | Appends `<PackageName>` to install location of dependencies, i.e. `${H5PP_DEPS_INSTALL_DIR}/<PackageName>`. This allows simple removal |
| `H5PP_ENABLE_PCH`         | `OFF`                  | Use precompiled headers to speed up compilation of tests and examples                                                                  |
| `H5PP_ENABLE_CCACHE`      | `OFF`                  | Use ccache to speed up compilation of tests and examples                                                                               |
| `H5PP_ENABLE_TESTS`       | `OFF`                  | Build tests (recommended!)                                                                                                             |
| `H5PP_BUILD_EXAMPLES`     | `OFF`                  | Build example programs                                                                                                                 |
| `H5PP_IS_SUBPROJECT`      | `OFF`                  | Use `h5pp` with add_subdirectory(). Skips installation of targets if true. Automatic detection if not set                              |
| `CONAN_PREFIX`            | None                   | conan install directory                                                                                                                |

In addition, variables such
as [`<PackageName>_ROOT`](https://cmake.org/cmake/help/latest/variable/PackageName_ROOT.html)
and [`<PackageName>_DIR`](https://cmake.org/cmake/help/latest/command/find_package.html) can be set to help guide
CMake's `find_package` calls:

### Option 3: Copy the headers

Copy the files under `h5pp/source/include` and add `#include<h5pp/h5pp.h>`. Make sure to compile
with `-std=c++17 -lstdc++fs` and link the dependencies `HDF5`, `Eigen3`, `fmt`, and `spdlog`. The actual linking is a
non-trivial step, see [linking](https://github.com/DavidAce/h5pp/wiki/Link-to-h5pp#link-using-cmake-targets-easy).

#### Compiler flags for Windows / MSVC

To get h5pp working in MSVC the following compiler flags and definitions are recommended

* `/permissive-`
* `/EHsc`
* `/D NOMINMAX`

If you use CMake to install/find`h5pp`, then no action is required since these are added automatically to the CMake
target `h5pp::h5pp`.


## Link

### Link using CMake targets (easy)

`h5pp` is easily imported into your project using CMake's `find_package`. Just point it to the `h5pp` install directory.
When found, targets are made available to compile and link to dependencies correctly. A minimal `CMakeLists.txt` to
use `h5pp` would look like:

```cmake
cmake_minimum_required(VERSION 3.15)
project(myProject)
add_executable(myExecutable main.cpp)
find_package(h5pp REQUIRED)                              # Define H5PP_ROOT to guide this search
target_link_libraries(myExecutable PRIVATE h5pp::h5pp)
```

#### Targets explained

* `h5pp::h5pp` is the main target including "everything" and should normally be the only target that you need --
  headers,flags and (if enabled) the found/downloaded dependencies.
* `h5pp::headers` links the `h5pp` headers only.
* `h5pp::deps` collects targets to link all the dependencies that were found/downloaded when `h5pp` was installed. These
  can of course be used independently.
   * If `H5PP_PACKAGE_MANAGER==find|cmake|fetch|cmp|conan` the targets are `Eigen3::Eigen`,`fmt::fmt`, `spdlog::spdlog`
     and `HDF5::HDF5`
* `h5pp::flags` sets compile and linker flags to enable C++17 and std::filesystem library, i.e. `-std=c++17`
  and `-lstdc++fs` (only needed on some compilers).

  Additionally, on MSVC:
   * `/permissive-`  to enable logical `and`/`or` in C++.
   * `/EHsc`
   * `/D NOMINMAX`

### Link manually (not as easy)

#### Using command line

From the command-line you can of course link using linker flags such as

```bash
 `g++ ... -std=c++17 -leigen3 -lspdlog  -lfmt -lhdf5_hl -lhdf5 -lstdc++fs  -pthread -lz -lsz -laec -lm -ldl  ...` 
```

provided these flags make sense on your system.

## Uninstall

The target `uninstall` is defined by `h5pp` which removes installed headers and dependencies using their respective
install manifests. From the build directory, run the following in the command-line to uninstall:

```
    cmake --build .  --target uninstall
```