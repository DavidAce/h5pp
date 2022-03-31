# Installation

## Get h5pp

There are currently 4 ways to obtain `h5pp`:

* `git clone https://github.com/DavidAce/h5pp.git` and install (see below)
* From [conan-center](https://conan.io/center/h5pp/1.9.0)
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

See full installation examples under [quickstart](https://github.com/DavidAce/h5pp/tree/master/quickstart). Find a
summary below.

### Option 1: Install with Conan (Recommended)

Install and configure [conan](https://conan.io), then run the following command to install from [conan center](https://conan.io/center/h5pp):

```bash
> conan install h5pp/1.9.0@ --build=missing
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

#### Opt-in automatic dependency installation with CMake

The CMake flag `H5PP_PACKAGE_MANAGER` controls the automated behavior for finding or installing dependencies. It can
take one of these string values:

| Value                | Description                                                                                                                                          |
|----------------------|------------------------------------------------------------------------------------------------------------------------------------------------------|
| `none`               | Skip handling dependencies                                                                                                                           |
| `find` **(default)** | Use CMake's `find_package`  to find dependencies                                                                                                     |
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

#### Using CMake

You could also use CMake's `find_package(...)` mechanism. The difficult part is linking to HDF5 libraries and its
dependencies. A minimal `CMakeLists.txt` could be something like this:

```cmake
cmake_minimum_required(VERSION 3.15)
project(myProject)

add_executable(myExecutable main.cpp)
target_include_directories(myExecutable PRIVATE <path-to-h5pp-headers>)
# Setup h5pp
target_compile_features(myExecutable PRIVATE cxx_std_17)
target_link_libraries(myExecutable PRIVATE stdc++fs) # To get <filesystem> headers working before GCC 9.1 

# For MSVC
target_compile_options(myExecutable INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive->) # and/or logical operators on VS
target_compile_options(myExecutable INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)        # try/catch without warnings on VS
target_compile_definitions(myExecutable INTERFACE $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>) # For std::min and std::max

# HDF5 install from source is recommended. In that case append CONFIG to find_package below. 
# Otherwise read more about this the native find_package module here:
#     https://cmake.org/cmake/help/latest/module/FindHDF5.html
find_package(HDF5 1.8 COMPONENTS C HL REQUIRED)  # Note that h5pp only needs the C libs of HDF5.


# CMake versions >= 3.19 bundle FindHDF5.cmake module that defines targets to use with find_package.
# However, these targets are not very helpful since they only define the imported libraries 
# (libhdf5.<so/a/dll/lib/dylib>), and not the rest of interface libraries such as `zlib, szip, libaec,
# dl, pthread`. To get these you can either interrogate your HDF5 compiler executable to get the full
# link line (`h5cc -show`), or just use the defined CMake variables:
target_link_libraries(myExecutable PRIVATE ${HDF5_LIBRARIES})
target_compile_definitions(myExecutable PRIVATE ${HDF5_DEFINITIONS})
target_include_directories(myExecutable PRIVATE ${HDF5_INCLUDE_DIR})


# The other dependencies lack find_package modules bundled with CMake, so this can be trickier.
# You can
#   1) Use find_package() to find installed packages in config-mode in your system
#   2) Use find_library() + add_library() to find libfmt, libspdlog in your system.
#   3) Just link -lfmt, -lspdlog and hope that these libraries are found by the linker.
target_link_libraries(myExecutable PRIVATE spdlog fmt)
target_include_directories(myExecutable PRIVATE <path-to-Eigen3-include-dir>)
target_include_directories(myExecutable PRIVATE <path-to-fmt-include-dir>)
target_include_directories(myExecutable PRIVATE <path-to-spdlog-include-dir>)

```


#### Use the custom FindHDF5.cmake bundled with `h5pp`

When installing `h5pp`, finding HDF5 and setting up the CMake target `HDF5::HDF5` for linking is handled by a custom
module for finding HDF5, defined in `cmake/modules/FindHDF5.cmake`. This module wraps the default `FindHDF5.cmake` which
comes with CMake and uses the same call signature, but fixes some annoyances with naming conventions in different
versions of CMake and HDF5 executables. It reads hints passed through CMake flags to find HDF5 somewhere on your
system (can be installed via,`apt`,`yum`, `brew`, `Easybuild`, etc) and defines a CMake target `HDF5::HDF5` with
everything you need to link correctly. Most importantly, it avoids injecting shared versions of libraries (dl, zlib,
szip, aec) during static builds. You can use the custom module too. Add the path pointing to `FindHDF5.cmake` to the
variable `CMAKE_MODULE_PATH` from within your own project:

```cmake
list(APPEND CMAKE_MODULE_PATH path/to/h5pp/cmake/modules/FindHDF5.cmake) # Replaces the bundled FindHDF5.cmake module
find_package(HDF5 1.10 COMPONENTS C HL REQUIRED)
if (TARGET HDF5::HDF5)
    target_link_libraries(myExecutable PRIVATE HDF5::HDF5)
endif ()
```

These are variables that can be used to guide the custom `FindHDF5.cmake` module:

| Var                 | Where     | Description                                                                                          |
|---------------------|-----------|------------------------------------------------------------------------------------------------------|
| `CMAKE_MODULE_PATH` | CMake     | List of directories where `CMake` should search for find-modules                                     |
| `CMAKE_PREFIX_PATH` | CMake     | List of directories where `find_package` should look for dependencies                                |
| `HDF5_ROOT`         | CMake/ENV | Path to HDF5 root install directory                                                                  |
| `HDF5_FIND_DEBUG`   | CMake     | Prints more information about the search for HDF5. See also `HDF5_FIND_DEBUG` in the original module |
| `EBROOTHDF5`        | ENV       | Variable defined by Easybuild with `module load HDF5`                                                |

## Uninstall

The target `uninstall` is defined by `h5pp` which removes installed headers and dependencies using their respective
install manifests. From the build directory, run the following in the command-line to uninstall:

```
    cmake --build .  --target uninstall
```