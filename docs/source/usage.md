# Usage

Using `h5pp` is intended to be simple. After initializing a file, most of the work can be done with
`file.dataset("path").write(data)` and `file.dataset("path").read<T>()`.

## Examples

```{include} ../../README.md
:start-after: "## Examples"
:end-before: "## Migration"
```

## File Access

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

## Storage Layout

HDF5 offers three [storage layouts](https://support.hdfgroup.org/HDF5/Tutor/layout.html#lo-define):

* `H5D_COMPACT`:  For scalar or small datasets which can fit in the metadata header. Default on datasets smaller than 32
  KB.
* `H5D_CONTIGUOUS`: For medium size datasets. Default on datasets smaller than 512 KB.
* `H5D_CHUNKED`: For large datasets. Default on datasets larger than 512 KB. This layout has some additional features:
    * Chunking, portioning of the data to improve IO performance by caching more efficiently. Chunk dimensions are
      calculated by `h5pp` if not given by the user.
    * Compression, disabled by default, and only available if HDF5 was built with zlib enabled.
    * Resize datasets. Note that the file size never decreases, for instance after overwriting with a smaller dataset.

`h5pp` can automatically determine the storage layout for each new dataset. To specify the layout when
creating a dataset, use `DatasetCreateOptions`:

```c++
    h5pp::DatasetCreateOptions create;
    create.h5Layout = H5D_CHUNKED;
    file.dataset("science/myChunkedData").ensure(create).write(myData);
```

## Compression

Chunked datasets can be compressed if HDF5 was built with zlib support. Use these functions to set or check the
default compression level for new chunked datasets:

```c++
    file.setCompressionLevel(3); // 0 disables compression; 1 to 9 enables it
    int compressionLevel = file.getCompressionLevel();
    bool compressionAvailable = h5pp::hdf5::isCompressionAvaliable();
```

To set the compression level when creating a particular dataset, use `DatasetCreateOptions`:

```c++
    h5pp::DatasetCreateOptions create;
    create.h5Layout = H5D_CHUNKED;
    create.compression = 3;
    file.dataset("science/myCompressedData").ensure(create).write(myData);
```

## Debug and logging

`h5pp` uses [spdlog](https://github.com/gabime/spdlog) to emit messages to stdout about its internal state during
read/write operations.
There are 7 levels of verbosity:

* `0: trace` (highest)
* `1: debug`
* `2: info`  (default)
* `3: warn`
* `4: error`
* `5: critical` (lowest)
* `6: off`

Set the level when constructing `h5pp::File`:

```c++
    h5pp::File file("myDir/someFile.h5", h5pp::FileAccess::REPLACE, h5pp::LogLevel::debug);
```

**NOTE:** Logging works the same with or without [spdlog](https://github.com/gabime/spdlog) enabled. When spdlog is *
not* found, a hand-crafted logger is used in its place to give identical output but without any performance
considerations (implemented with STL lists, strings and streams).

## Tips

### **NEW:** [h5du](https://github.com/DavidAce/h5du)

List the size of objects inside an HDF5 file with [h5du](https://github.com/DavidAce/h5du).

### View data

Try [HDF Compass](https://support.hdfgroup.org/projects/compass)
or [HDFView](https://www.hdfgroup.org/downloads/hdfview). Both are available in Ubuntu's package repository.

### Load data into Python

HDF5 data is easy to load into Python using [h5py](https://docs.h5py.org/en/stable). Loading integer and floating point
data is straightforward. Complex data is almost as simple, so let's use that as an example.

HDF5 does not support complex types natively, but `h5pp`enables this by using a custom compound HDF5 type with `real`
and `imag` fields. Here is a python example which uses [h5py](https://docs.h5py.org/en/stable) to load 1D arrays from an
HDF5 file generated with `h5pp`:

```python
import h5py
import numpy as np

file = h5py.File('myFile.h5', 'r')

# previously written as std::vector<double> in h5pp
myDoubleArray = file['double-array-dataset'][()]

# previously written as std::vector<std::complex<double>> in h5pp
myComplexArray = file['complex-double-array-dataset'][()].view(dtype=np.complex128)
```

Notice the cast to `dtype=np.complex128` which interprets each element of the array as two `doubles`, i.e. the real and
imaginary parts are `2 * 64 = 128` bits.