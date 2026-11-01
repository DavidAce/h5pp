# Installation

These instructions describe h5pp 2.0.0 on the `v2` branch. The version in
ConanCenter or an older release archive may differ; the commands below use
this checkout rather than assume that h5pp 2.0.0 has been published.

## Requirements

* A C++20 compiler and standard library. `H5PP_USE_FLOAT128` requires C++23.
* CMake 3.24 or newer. The bundled CMake dependency provider requires 3.26
  because it builds HDF5 2.1.1, whose own minimum is CMake 3.26.
* HDF5 1.8 or newer, including the C and high-level C libraries. Older HDF5
  installations remain supported; direct chunk I/O requires HDF5 1.10.5 or newer.
* Conan 2.0.5 or newer, only when using the Conan recipe or dependency provider.

### Optional dependencies

| Dependency | Minimum supported version | Enable with CMake |
|------------|---------------------------|-------------------|
| Eigen | 3.4.0 | `H5PP_ENABLE_EIGEN3=ON` |
| fmt | 9.1.0 | `H5PP_ENABLE_FMT=ON` |
| spdlog | 1.12.0 | `H5PP_ENABLE_SPDLOG=ON` (also enables fmt) |
| Catch2 | 3 | `H5PP_ENABLE_TESTS=ON` (development only) |

The minimum supported version is different from the version selected by a
provider. The bundled CMake provider currently downloads Eigen 5.0.1, fmt
12.1.0, spdlog 1.17.0, HDF5 2.1.1, zlib 1.3.1, libaec 1.1.7, and Catch2
3.13.0 when tests are enabled. Those download choices do not raise the library
minimums above. For example, an existing Eigen 3.4.0 installation can be used
without the provider even though the provider downloads Eigen 5.0.1.

The Conan recipe selects Eigen 5.0.1, spdlog in `[>=1.17.0 <1.18]`, HDF5 in
`[>=1.10.0 <1.15]`, and zlib in `[>=1.2.11 <2]`. It requests Catch2 3.13.0
when tests are enabled; fmt and libaec are resolved through its dependencies.
These recipe constraints describe the Conan installation path, not every
version usable with manually installed dependencies. In particular, the
current Conan path selects HDF5 1.x, while the CMake provider selects HDF5 2.x.

Without fmt or spdlog enabled, h5pp uses its built-in formatting and logging.
Header-only users can enable optional integrations with `H5PP_USE_EIGEN3`,
`H5PP_USE_FMT`, and `H5PP_USE_SPDLOG` preprocessor definitions and supply the
corresponding headers and libraries themselves.

### 128-bit floating-point options

Both options are off by default. Choose at most one:

* `H5PP_USE_QUADMATH=ON` uses GCC's `__float128` and `__complex128` with
  libquadmath. It keeps h5pp's C++20 minimum. Configuration checks that the
  type, header, and library can be used together.
* `H5PP_USE_FLOAT128=ON` uses `std::float128_t` from `<stdfloat>` and
  `std::complex<std::float128_t>`. It raises the h5pp target's minimum to C++23.
  Configuration compiles h5pp's float128 header and fails immediately if the
  selected compiler and standard library do not provide the required types.

No compiler-version table is used to decide whether these types are available.
Header-only users who do not use CMake select the implementation with the
corresponding preprocessor definition.

## Install from this checkout

```bash
git clone --branch v2 https://github.com/DavidAce/h5pp.git
cd h5pp
```

### Use dependencies already installed on your system

Without a dependency provider, h5pp uses `find_package` to locate dependencies.
Use `CMAKE_PREFIX_PATH` or package-specific hints such as `HDF5_ROOT`,
`Eigen3_DIR`, `fmt_DIR`, and `spdlog_DIR` for installations outside the usual
search paths.

```bash
cmake -S . -B build/local \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PWD/install/local" \
    -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
    -DH5PP_ENABLE_EIGEN3=ON \
    -DH5PP_ENABLE_SPDLOG=ON
cmake --build build/local --parallel 6
cmake --install build/local
```

Optional integrations are off by default in a plain configuration; enable
only the ones your project needs. HDF5 is always required. The library is
header-only, so a build without tests, examples, or documentation has no h5pp
source files to compile.

### Let CMake download and build dependencies

With CMake 3.26 or newer, use the bundled provider through a preset:

```bash
cmake --preset release-cmake
cmake --build --preset release-cmake --parallel 6
cmake --install build/release-cmake
```

This preset enables optional integrations, tests, and examples. Its installation
prefix is `install/release-cmake` under the source directory. To register the
provider without a preset, pass it on the first configuration of a new build:

```bash
cmake -S . -B build/provider \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$PWD/cmake/cmake_dependency_provider/cmake_provider.cmake" \
    -DCMAKE_INSTALL_PREFIX="$PWD/install/provider" \
    -DH5PP_ENABLE_EIGEN3=ON \
    -DH5PP_ENABLE_SPDLOG=ON
```

The provider builds dependencies during configuration and installs them under
`CMAKE_INSTALL_PREFIX`. It reuses installations recorded by its success
manifests. Use a separate build and installation prefix when changing dependency
providers or configurations.

### Let Conan install dependencies

Install Conan 2.0.5 or newer and put `conan` on `PATH`. If you have no default
profile yet, create one with `conan profile detect`. Then use the bundled preset:

```bash
cmake --preset release-conan
cmake --build --preset release-conan --parallel 6
cmake --install build/release-conan
```

The preset registers `cmake/conan_dependency_provider/conan_provider.cmake`
through `CMAKE_PROJECT_TOP_LEVEL_INCLUDES`. The provider reads this checkout's
`conanfile.py` and generates dependency targets. It does not download h5pp itself
from ConanCenter.

To package this checkout into your local Conan cache instead:

```bash
conan create . --build=missing -s:h compiler.cppstd=20
```

Use `compiler.cppstd=23` when selecting the recipe's `with_float128=True` option.
A project can then require `h5pp/2.0.0` from that local cache. The
[cmake-conan quickstart](https://github.com/DavidAce/h5pp/tree/v2/quickstart/cmake-conan-integration)
lets CMake call Conan automatically to install the packages listed in its
`conanfile.txt`, using configured remotes or the local cache.

`H5PP_PACKAGE_MANAGER` and `H5PP_DOWNLOAD_METHOD` are deprecated. Select a
provider with a preset or `CMAKE_PROJECT_TOP_LEVEL_INCLUDES`; omit a provider
to use dependencies you have installed yourself. The old `none`, `fetch`,
`cpm`, and `find-or-*` settings are not installation modes in v2.

### Copy the headers

Copy the `include/h5pp` directory into your project's include path and include
`<h5pp/h5pp.h>`. Compile as C++20 or newer, or C++23 for `H5PP_USE_FLOAT128`.
Supply HDF5's C and high-level C libraries and their dependencies. Eigen is
header-only; fmt and spdlog may require libraries depending on their own build
configuration. CMake targets are recommended for handling these link details.

## CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `H5PP_ENABLE_EIGEN3` | `OFF` | Enable Eigen integration |
| `H5PP_ENABLE_FMT` | `OFF` | Enable fmt formatting |
| `H5PP_ENABLE_SPDLOG` | `OFF` | Enable spdlog logging and fmt |
| `H5PP_USE_QUADMATH` | `OFF` | Use GCC 128-bit types with libquadmath |
| `H5PP_USE_FLOAT128` | `OFF` | Use C++23 `std::float128_t` |
| `H5PP_ENABLE_TESTS` | `OFF` | Build the existing Catch2 tests |
| `H5PP_BUILD_EXAMPLES` | `OFF` | Build examples |
| `H5PP_BUILD_DOCS` | `OFF` | Build documentation with Doxygen and Sphinx |
| `H5PP_ENABLE_PCH` | `OFF` | Use precompiled headers for tests/examples |
| `H5PP_ENABLE_CCACHE` | `OFF` | Use ccache for development builds |
| `H5PP_ENABLE_ASAN` | `OFF` | Enable AddressSanitizer |
| `H5PP_ENABLE_COVERAGE` | `OFF` | Enable coverage instrumentation for tests |
| `H5PP_ENABLE_MPI` | `OFF` | Enable experimental MPI support; requires parallel HDF5 |
| `H5PP_IS_SUBPROJECT` | Automatically detected | Use h5pp with `add_subdirectory`; skip installation rules |
| `CMAKE_INSTALL_PREFIX` | CMake's default | Install h5pp, and dependencies built by the CMake provider |

Presets override several defaults. Run `cmake --list-presets` to see available
configurations. h5pp does not impose one static/shared choice on all dependencies;
use the options supported by the selected packages and provider.

## Link from a CMake project

```cmake
cmake_minimum_required(VERSION 3.24)
project(myProject LANGUAGES C CXX)
find_package(h5pp 2.0.0 REQUIRED)
add_executable(myExecutable main.cpp)
target_link_libraries(myExecutable PRIVATE h5pp::h5pp)
```

Configure your project with `-DCMAKE_PREFIX_PATH=/path/to/h5pp/install/prefix`.
The installed package finds HDF5 and the optional dependencies enabled when h5pp
was installed. Their headers and libraries must still be available.

* `h5pp::h5pp` supplies the headers, dependencies, and compiler requirements.
  This is normally the only target your executable needs.
* `h5pp::headers` supplies the h5pp include directory.
* `h5pp::deps` supplies HDF5 and enabled optional integrations. `h5pp::hdf5`
  supplies h5pp's self-contained HDF5 target, including the C and high-level APIs.
* `h5pp::flags` supplies the C++20 requirement, or C++23 in standard float128
  mode, and enabled compiler options and definitions. On MSVC this includes
  `/permissive-`, `/EHsc`, and `NOMINMAX`.

Eigen is header-only: do not add `-leigen3`. HDF5's extra link libraries depend
on how it was built; use its package targets rather than copying a fixed list
of `-lz`, `-lsz`, `-ldl`, and similar flags from another system.

## Uninstall

To remove files listed in h5pp's installation manifest:

```bash
cmake --build build/local --target h5pp-uninstall
```

This does not uninstall separately managed system or Conan dependencies.
