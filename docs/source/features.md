# Features
* Header-only C++17 template library.
* High-level front-end to the C API of the HDF5 library.
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
    * Structs as HDF5 Compound types ([example](https://github.com/DavidAce/h5pp/blob/master/examples/example-04a-custom-struct-easy.cpp))
    * Structs as HDF5 Tables (with user-defined compound HDF5 types for entries)
    * Ragged "variable-length" data in HDF5 Table columns using `h5pp::varr_t<>` and `h5pp::vstr_t`.
* Modern CMake installation of `h5pp` and (opt-in) installation of dependencies.
* Multi-platform: Linux, Windows, OSX. (Developed under Linux).
