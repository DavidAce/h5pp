# Features
* Header-only C++17 template library
* High-level front-end to the C API of HDF5
* Modern CMake installation of `h5pp` and its dependencies (optional)
* Multi-platform: Linux, Windows, OSX. (Developed under Linux)
* Supports:
    *  all numeric types: `(u)int#_t`, `float`, `double`, `long double`
    *  **`std::complex<>`** with any of the types above
    *  CUDA-style POD-structs with `x,y` or `x,y,z` members as atomic type, such as `float3` or `double2`. These work with any of the types above. In `h5pp` these go by the name `Scalar2<>` and `Scalar3<>`.
    *  Contiguous containers with a `.data()` member, such as `std::vector<>`
    *  Text types `std::string`, `char` arrays, and `std::vector<std::string>`
    *  C-style arrays or pointer to buffer
    *  [**Eigen**](http://eigen.tuxfamily.org) types such as `Eigen::Matrix<>`, `Eigen::Array<>` and `Eigen::Tensor<>`, with automatic conversion to/from row-major storage
    *  Structs as compound HDF5 types (see [example](https://github.com/DavidAce/h5pp/blob/master/examples/example-04a-custom-struct-easy.cpp))
    *  Structs as HDF5 tables (with user-defined compound HDF5 types for entries)

