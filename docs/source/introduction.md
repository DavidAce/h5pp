# Introduction

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
* Simple options common tasks, e.g. for file access, storage layout, hyperslabs, chunking and compression.
* Simple installation with modular dependencies and opt-in automation.
* Simple documentation (work in progress).
