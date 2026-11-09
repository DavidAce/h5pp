# Quick Start

These examples use h5pp 2.0.0 from the `v2` branch and require a C++20 toolchain.
The standard float128 option requires C++23 instead. See the
[installation guide](../docs/source/installation.md) for dependency minimums
and the different versions selected by each provider.

* `git-clone-cmake-link-deps-manually`: requires CMake >= 3.26 and preinstalled
  HDF5, Eigen >= 3.4.0, fmt >= 9.1.0, and spdlog >= 1.12.0. Run
  `step1-install-h5pp.sh`, then `step2-build-run-project.sh`. The installed
  h5pp targets supply the dependency links; no deprecated package-manager
  setting is needed.
* `git-clone-install-with-conanfile.py`: requires Conan >= 2.0.5 and a default
  profile. `install-h5pp.sh` creates a local Conan package from the v2 checkout.
* `cmake-conan-integration`: requires CMake >= 3.26 and Conan >= 2.0.5 on `PATH`.
  `build-and-run-project.sh` configures the project with CMake; the bundled
  [cmake-conan](https://github.com/conan-io/cmake-conan/) provider calls Conan
  automatically on the first `find_package()`. The script then builds and
  runs the executable. The h5pp version listed in `MyProject/conanfile.txt`
  must be available from a configured Conan remote or your local cache.
  For an unpublished v2 package, the separate Conan packaging example above
  can prepare that cache entry.

Run `conan profile detect` if you do not yet have a default Conan profile.
The Conan examples explicitly select C++20. For custom dependency locations,
set `CMAKE_PREFIX_PATH` before running the CMake example scripts.

To build h5pp itself with the bundled CMake provider, use `release-cmake` from
the repository root with CMake >= 3.26; that provider builds HDF5 2.1.1.
