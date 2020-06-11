#pragma once

#include "h5ppHid.h"
#include "h5ppOptional.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <vector>
namespace h5pp {
    class TableProperties {
        public:
        hid::h5t                                entryType;
        std::optional<std::string>              tableTitle;
        std::optional<std::string>              tableName;
        std::optional<std::string>              groupName;
        std::optional<hsize_t>                  NFIELDS;
        std::optional<hsize_t>                  NRECORDS;
        std::optional<hsize_t>                  chunkSize;
        std::optional<size_t>                   entrySize;
        std::optional<size_t>                   compressionLevel;
        std::optional<std::vector<size_t>>      fieldOffsets;
        std::optional<std::vector<size_t>>      fieldSizes;
        std::optional<std::vector<hid::h5t>>    fieldTypes;
        std::optional<std::vector<std::string>> fieldNames;
    };
} // namespace h5pp
