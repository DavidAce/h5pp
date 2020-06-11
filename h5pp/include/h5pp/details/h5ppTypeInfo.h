#pragma once
#include "h5ppHid.h"
#include "h5ppUtils.h"
#include <string>
#include <typeindex>

namespace h5pp {
    /*!
     * \brief Collects type information about existing datasets
     */
    struct TypeInfo {
        std::optional<std::string>          cpp_type_name;
        std::optional<size_t>               cpp_type_bytes;
        std::optional<std::type_index>      cpp_type_index;
        std::optional<std::string>          h5_path;
        std::optional<std::string>          h5_name;
        std::optional<hsize_t>              h5_size;
        std::optional<int>                  h5_rank;
        std::optional<std::vector<hsize_t>> h5_dims;
        std::optional<hid::h5t>             h5_type;
        std::optional<hid::h5o>             h5_link;

        [[nodiscard]] std::string string() {
            std::string msg;
            if(cpp_type_name) msg.append(h5pp::format("C++: type [{}]", cpp_type_name.value()));
            if(cpp_type_bytes) msg.append(h5pp::format(" bytes [{}]", cpp_type_bytes.value()));
            if(not msg.empty()) msg.append(" | HDF5:");
            if(h5_path) msg.append(h5pp::format(" path [{}]", h5_path.value()));
            if(h5_name) msg.append(h5pp::format(" name [{}]", h5_name.value()));
            if(h5_size) msg.append(h5pp::format(" size [{}]", h5_size.value()));
            if(h5_rank) msg.append(h5pp::format(" rank [{}]", h5_rank.value()));
            if(h5_dims) msg.append(h5pp::format(" dims {}", h5_dims.value()));
            return msg;
        }
    };

    /*!
     * \brief Collects type information about existing tables
     */
    struct TableTypeInfo {
        std::optional<size_t>                   numFields;
        std::optional<size_t>                   numRecords;
        std::optional<size_t>                   recordBytes;
        std::optional<std::vector<std::string>> fieldNames;
        std::optional<std::vector<size_t>>      fieldSizes;
        std::optional<std::vector<size_t>>      fieldOffsets;
        std::optional<std::vector<hid::h5t>>    fieldTypes;
        std::optional<std::string>              tableName;
        std::optional<hid::h5f>                 tableFile;
        std::optional<hid::h5g>                 tableGroup;
        std::optional<hid::h5d>                 tableDset;
        std::optional<hid::h5t>                 tableType;
        std::optional<size_t>                   compressionLevel;
        void assertReadReady() const {
//            if(not )
        }
        void assertWriteReady() const {
//            if(not )
        }
    };

}
