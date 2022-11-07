#pragma once
#include "h5ppTypeSfinae.h"
#include <H5Tpublic.h>
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
      * | `READONLY`                | Open with read-only access           | Throw error     | Never writes to disk, fails if the file is not found |
      * | `COLLISION_FAIL`          | Throw error                          | Create new file | Never deletes existing files and fails if it already exists |
      * | `RENAME` **default**      | Create renamed file                  | Create new file | Never deletes existing files. Invents a new filename to avoid collision by appending "-#" (#=1,2,3...) to the stem of the filename |
      * | `READWRITE`               | Open with read-write access          | Create new file | Never deletes existing files, but is allowed to open/modify |
      * | `BACKUP`                  | Rename existing file and create new  | Create new file | Avoids collision by backing up the existing file, appending `.bak_#` (`#`=1,2,3...) to the filename |
      * | `REPLACE`                 | Truncate (overwrite)                 | Create new file | Deletes the existing file and create a new one in place |
      *
      *
      * - When a new file is created, the intermediate directories are always created automatically.
      * - When a new file is created, `READWRITE` access to it is implied.
      *
      */
    enum class FileAccess {
        READONLY,           /* Never writes to disk, fails if the file is not found */
        COLLISION_FAIL,     /* Never deletes existing files and fails if it already exists */
        RENAME,             /* Never deletes existing files. Invents a new filename to avoid collision by appending "-#" (#=1,2,3...) to the stem of the filename */
        READWRITE,          /* Never deletes existing files, but is allowed to open/modify */
        BACKUP,             /* Avoids collision by backing up the existing file, appending ".bak_#" (#=1,2,3...) to the filename */
        REPLACE             /* Deletes the existing file and create a new one in place */
    };
    using FilePermission = FileAccess;
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
        if constexpr(std::is_same_v<T, LogLevel>) return static_cast<T>(l);
        else return std::min(static_cast<T>(l), static_cast<T>(6));
    }

    template<typename T>
    constexpr LogLevel Num2Level(T l) {
        static_assert(std::is_same_v<T, LogLevel> or std::is_integral_v<T>);
        if constexpr(std::is_same_v<T, LogLevel>) return l;
        else return static_cast<LogLevel>(std::min(l, static_cast<T>(6)));
    }

    template<typename T>
    constexpr bool operator<=(LogLevel level, T num) {
        static_assert(std::is_integral_v<T> or std::is_same_v<T, LogLevel>);
        using utype = typename std::underlying_type<LogLevel>::type;
        return Level2Num(level) <= static_cast<utype>(num);
    }

    template<typename T>
    constexpr bool operator==(LogLevel level, T num) {
        static_assert(std::is_integral_v<T> or std::is_same_v<T, LogLevel>);
        using utype = typename std::underlying_type<LogLevel>::type;
        return Level2Num(level) == static_cast<utype>(num);
    }
    template<typename T>
    constexpr std::string_view enum2str(const T &item) {
        /* clang-format off */
        static_assert(std::is_enum_v<T>);
        static_assert(type::sfinae::is_any_v<T,
        FileAccess,
        TableSelection,
        ResizePolicy,
        LogLevel,
        H5T_class_t>);
        if constexpr(std::is_same_v<T, FileAccess>) switch(item) {
            case FileAccess::READONLY:       return "READONLY";
            case FileAccess::COLLISION_FAIL: return "COLLISION_FAIL";
            case FileAccess::RENAME:         return "RENAME";
            case FileAccess::READWRITE:      return "READWRITE";
            case FileAccess::BACKUP:         return "BACKUP";
            case FileAccess::REPLACE:        return "REPLACE";
        }
        else if constexpr(std::is_same_v<T, TableSelection>) switch(item) {
            case TableSelection::FIRST:      return "FIRST";
            case TableSelection::LAST:       return "LAST";
            case TableSelection::ALL:        return "ALL";
        }
        else if constexpr(std::is_same_v<T, ResizePolicy>) switch(item) {
            case ResizePolicy::FIT:          return "FIT";
            case ResizePolicy::GROW:         return "GROW";
            case ResizePolicy::OFF:          return "OFF";
        }
        else if constexpr(std::is_same_v<T, LocationMode>) switch(item) {
            case LocationMode::SAME_FILE:    return "SAME_FILE";
            case LocationMode::OTHER_FILE:   return "OTHER_FILE";
            case LocationMode::DETECT:       return "DETECT";
        }
        else if constexpr(std::is_same_v<T, LogLevel>) switch(item) {
            case LogLevel::trace:            return "trace";
            case LogLevel::debug:            return "debug";
            case LogLevel::info:             return "info";
            case LogLevel::warn:             return "warn";
            case LogLevel::err:              return "err";
            case LogLevel::critical:         return "critical";
            case LogLevel::off:              return "off";
        }
        else if constexpr(std::is_same_v<T, H5T_class_t>) switch(item) {
            case H5T_class_t::H5T_NO_CLASS:  return "H5T_NO_CLASS";
            case H5T_class_t::H5T_INTEGER:   return "H5T_INTEGER";
            case H5T_class_t::H5T_FLOAT:     return "H5T_FLOAT";
            case H5T_class_t::H5T_TIME:      return "H5T_TIME";
            case H5T_class_t::H5T_STRING:    return "H5T_STRING";
            case H5T_class_t::H5T_BITFIELD:  return "H5T_BITFIELD";
            case H5T_class_t::H5T_OPAQUE:    return "H5T_OPAQUE";
            case H5T_class_t::H5T_COMPOUND:  return "H5T_COMPOUND";
            case H5T_class_t::H5T_REFERENCE: return "H5T_REFERENCE";
            case H5T_class_t::H5T_ENUM:      return "H5T_ENUM";
            case H5T_class_t::H5T_VLEN:      return "H5T_VLEN";
            case H5T_class_t::H5T_ARRAY:     return "H5T_ARRAY";
            case H5T_class_t::H5T_NCLASSES:  return "H5T_NCLASSES";
            default:                         return "H5T_NO_CLASS";
        }
        else static_assert(type::sfinae::unrecognized_type_v<T> and "enum2str(): Not a known enum");
        /* clang-format on */
        return "UNKNOWN ENUM";
    }
}
