#pragma once

namespace h5pp {

    // Read more about the original HDF5 file permissions here:
    // https://portal.hdfgroup.org/display/HDF5/H5F_CREATE
    // and
    // https://portal.hdfgroup.org/display/HDF5/H5F_OPEN

    /*
     *
     `h5pp` offers more flags for file access permissions than HDF5. The new flags are primarily intended to
      prevent accidental loss of data, but also to clarify intent and avoid mutually exclusive options.

     The flags are listed in the order of increasing "danger" that they pose to previously existing files.

     | Flag | File exists | No file exists | Comment |
     | ---- | ---- | ---- | ---- |
     | `READONLY`                | Open with read-only permission       | Throw error     | Never writes to disk, fails if the file is not found |
     | `COLLISION_FAIL`          | Throw error                          | Create new file | Never deletes existing files and fails if it already exists |
     | `RENAME` **(default)**    | Rename this file and create          | Create new file | Never deletes existing files, but invents a new filename to avoid collision by appending
     "-#" (#=1,2,3...) to the stem of the filename | | `READWRITE`               | Open with read-write permission      | Create new file | Never deletes existing files, but is
     allowed to open/modify | | `BACKUP`                  | Rename existing file and create      | Create new file | Avoids collision by backing up the existing file, appending
     ".bak_#" (#=1,2,3...) to the filename | | `REPLACE`                 | Truncate (overwrite)                 | Create new file | Deletes the existing file and create a new one
     in place |

     * When a new file is created, the intermediate directories are always created automatically.
     * When a new file is created, `READWRITE` permission to it is implied.
     *
     */

    enum class FilePermission { READONLY, COLLISION_FAIL, RENAME, READWRITE, BACKUP, REPLACE };
}
