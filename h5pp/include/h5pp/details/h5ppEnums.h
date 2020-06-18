#pragma once

namespace h5pp {


    enum class FilePermission {

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
         | `RENAME` **(default)**    | Create renamed file                  | Create new file | Never deletes existing files. Invents a new filename to avoid collision by appending "-#" (#=1,2,3...) to the stem of the filename |
         | `READWRITE`               | Open with read-write permission      | Create new file | Never deletes existing files, but is allowed to open/modify |
         | `BACKUP`                  | Rename existing file and create new  | Create new file | Avoids collision by backing up the existing file, appending ".bak_#" (#=1,2,3...) to the filename |
         | `REPLACE`                 | Truncate (overwrite)                 | Create new file | Deletes the existing file and create a new one in place |


         * When a new file is created, the intermediate directories are always created automatically.
         * When a new file is created, `READWRITE` permission to it is implied.
         *
         */
        READONLY, COLLISION_FAIL, RENAME, READWRITE, BACKUP, REPLACE
    };

    enum class FileDriver{
        /*
        From the HDF5 manual:
        ------------------
        Supported file drivers in HDF5
        Driver Name         Driver Identifier   Description
        POSIX	 	        H5FD_SEC2	 	    This driver uses POSIX file-system functions like read and write to perform I/O to a single, permanent file on local disk with no system buffering. This driver is POSIX-compliant and is the default file driver for all systems.
        Direct	 	        H5FD_DIRECT	 	    This is the H5FD_SEC2 driver except data is written to or read from the file synchronously without being cached by the system.
        Log	 	            H5FD_LOG	 	    This is the H5FD_SEC2 driver with logging capabilities.	 	H5Pset_fapl_log
        Windows	 	        H5FD_WINDOWS	    This driver was modified in HDF5-1.8.8 to be a wrapper of the POSIX driver, H5FD_SEC2. This change should not affect user applications.
        STDIO	 	        H5FD_STDIO	 	    This driver uses functions from the standard C stdio.h to perform I/O to a single, permanent file on local disk with additional system buffering.
        Memory	 	        H5FD_CORE	 	    With this driver, an application can work with a file in memory for faster reads and writes. File contents are kept in memory until the file is closed. At closing, the memory version of the file can be written back to disk or abandoned.
        Family	 	        H5FD_FAMILY	 	    With this driver, the HDF5 file�s address space is partitioned into pieces and sent to separate storage files using an underlying driver of the user’s choice. This driver is for systems that do not support files larger than 2 gigabytes.
        Multi	 	        H5FD_MULTI	 	    With this driver, data can be stored in multiple files according to the type of the data. I/O might work better if data is stored in separate files based on the type of data. The Split driver is a special case of this driver.
        Split	 	        H5FD_SPLIT	 	    This file driver splits a file into two parts. One part stores metadata, and the other part stores raw data. This splitting a file into two parts is a limited case of the Multi driver.
        Parallel	        H5FD_MPIO	 	    This is the standard HDF5 file driver for parallel file systems. This driver uses the MPI standard for both communication and file I/O.
        */

        SEC2,DIRECT,LOG,WINDOWS,STDIO,CORE,FAMILY,MULTI,SPLIT,MPIO
    };


    enum class TableSelection {
        FIRST,  // First element in a table
        LAST,   // Last element in a table
        ALL,    // All elements of a table
    };

    enum class ResizeMode {
        RESIZE_TO_FIT, // (Default)
        INCREASE_ONLY,
        DO_NOT_RESIZE,
    };
}
