#pragma once

/*! \namespace h5pp
 * \brief A simple C++17 wrapper for the HDF5 library
 */

namespace h5pp {

    /* clang-format off */

     /*! \brief %File access permissions
      *
      * See the original file access permissions in the HDF5 Documentation
      * for [H5F_CREATE](https://portal.hdfgroup.org/display/HDF5/H5F_CREATE)
      * and [H5F_OPEN](https://portal.hdfgroup.org/display/HDF5/H5F_CREATE).
      *
      * `h5pp` offers more flags for file access permissions than HDF5. The new flags are primarily intended to
      * prevent accidental loss of data, but also to clarify intent and avoid mutually exclusive options.
      *
      * The flags are listed in the order of increasing "danger" that they pose to previously existing files.
      *
      * | Enumerator | %File exists | No file exists | Comment |
      * | ----  | ---- | ---- | ---- |
      * | `READONLY`                | Open with read-only permission       | Throw error     | Never writes to disk, fails if the file is not found |
      * | `COLLISION_FAIL`          | Throw error                          | Create new file | Never deletes existing files and fails if it already exists |
      * | `RENAME` **default**      | Create renamed file                  | Create new file | Never deletes existing files. Invents a new filename to avoid collision by appending "-#" (#=1,2,3...) to the stem of the filename |
      * | `READWRITE`               | Open with read-write permission      | Create new file | Never deletes existing files, but is allowed to open/modify |
      * | `BACKUP`                  | Rename existing file and create new  | Create new file | Avoids collision by backing up the existing file, appending `.bak_#` (`#`=1,2,3...) to the filename |
      * | `REPLACE`                 | Truncate (overwrite)                 | Create new file | Deletes the existing file and create a new one in place |
      *
      *
      * - When a new file is created, the intermediate directories are always created automatically.
      * - When a new file is created, `READWRITE` permission to it is implied.
      *
      */
    enum class FilePermission {
        READONLY,           /* Never writes to disk, fails if the file is not found */
        COLLISION_FAIL,     /* Never deletes existing files and fails if it already exists */
        RENAME,             /* Never deletes existing files. Invents a new filename to avoid collision by appending "-#" (#=1,2,3...) to the stem of the filename */
        READWRITE,          /* Never deletes existing files, but is allowed to open/modify */
        BACKUP,             /* Avoids collision by backing up the existing file, appending ".bak_#" (#=1,2,3...) to the filename */
        REPLACE             /* Deletes the existing file and create a new one in place */
    };

    /* clang-format on */

    /*! \brief Choose which row to read/write/copy/move on table operations
     */
    enum class TableSelection {
        FIRST, /*!< Selects the first element in a table */
        LAST,  /*!< Selects the last element in a table */
        ALL,   /*!< Selects the all elements in a table */
    };

    /*! \brief Set policy for modifying dataset dimensions when overwriting
     */
    enum class ResizePolicy {
        FIT,  /*!< Overwriting a dataset will shrink or grow existing dimensions to fit new data (default on H5D_CHUNKED) */
        GROW, /*!< Overwriting a dataset will may grow existing dimensions, but never shrink, to fit new data (works only on H5D_CHUNKED) */
        OFF,  /*!< Overwriting a dataset will not modify existing dimensions */
    };

    /*! \brief Specify whether the target location is on the same file or a different one when copying objects
     */
    enum class LocationMode {
        /*
         * Some operations, such as h5pp::hdf5::copyLink support copying objects between files.
         * However, detecting whether two given location ids are on the same file can become
         * a bottleneck when batch processing a large amount of files. If we know beforehand what it will be, setting
         * this flag avoids a costly detection step.
         */

        SAME_FILE,  /*!< Interpret source and target location id's as being on the same file */
        OTHER_FILE, /*!< Interpret source and target location id's as being on different files */
        DETECT,     /*!< Use H5Iget_file_id() to check. This is the default, but avoid when known. */
    };

    /*! \brief Mimic the log levels in spdlog
     */
    enum class LogLevel : size_t {
        trace    = 0ul,
        debug    = 1ul,
        info     = 2ul,
        warn     = 3ul,
        err      = 4ul,
        critical = 5ul,
        off      = 6ul,
    };

    template<typename LogLevelType, typename T = typename std::underlying_type<LogLevel>::type>
    constexpr T Level2Num(LogLevelType l) noexcept {
        static_assert(std::is_same_v<T, LogLevel> or std::is_integral_v<T>);
        if constexpr(std::is_same_v<T, LogLevel>)
            return static_cast<T>(l);
        else {
            return std::min(static_cast<T>(l), static_cast<T>(6));
        }
    }

    template<typename T>
    constexpr LogLevel Num2Level(T l) {
        static_assert(std::is_same_v<T, LogLevel> or std::is_integral_v<T>);
        if constexpr(std::is_same_v<T, LogLevel>)
            return l;
        else {
            return static_cast<LogLevel>(std::min(l, static_cast<T>(6)));
        }
    }

    template<typename T>
    constexpr bool operator<=(LogLevel level, T num) {
        static_assert(std::is_integral_v<T> or std::is_same_v<T, LogLevel>);
        return Level2Num<T>(level) <= num;
    }

    template<typename T>
    constexpr bool operator ==(LogLevel level, T num) {
        static_assert(std::is_integral_v<T> or std::is_same_v<T, LogLevel>);
        return Level2Num<T>(level) == num;
    }

}
