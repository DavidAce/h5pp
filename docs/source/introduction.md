# Introduction

[HDF5](https://www.hdfgroup.org/) is a popular format for portable binary storage of large datasets.
With bindings to languages such as Python, Julia, Matlab and many others,
it is straightforward to export, import and analyze data in a collaborative setting.

In C/C++ using HDF5 directly is not straightforward.
Beginners are met with a steep learning curve to the vast API of HDF5.
There are many C/C++ libraries already that simplify the user experience, but as a matter of opinion,
things could be even simpler.

`h5pp` makes HDF5 simple in the following sense:
*  Read and write common C++ types in a single line of code.
*  No prior knowledge of HDF5 is required.
*  Default settings let simple tasks stay simple, e.g., storage layout, chunking and compression.
*  Advanced tasks remain possible, e.g. MPI parallelism.
*  Meaningful logs and error messages even for beginners.
*  Simple installation with modular dependencies and opt-in automation.