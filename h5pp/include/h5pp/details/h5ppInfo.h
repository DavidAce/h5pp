#pragma once
#include "h5ppEnums.h"
#include "h5ppHid.h"
#include "h5ppLogger.h"
#include "h5ppOptional.h"
#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
#include <typeindex>
#include <variant>
#include <vector>

namespace h5pp {

    struct Options {
        std::optional<std::vector<hsize_t>>       dataDims          = std::nullopt;
        std::optional<std::vector<hsize_t>>       dataDimsMax       = std::nullopt;
        std::optional<std::vector<hsize_t>>       chunkDims         = std::nullopt;
        std::optional<hid::h5t>                   h5_type           = std::nullopt;
        std::optional<H5D_layout_t>               h5_layout         = std::nullopt;
        std::optional<unsigned int>               compression       = std::nullopt;
        std::optional<std::vector<HyperSlab>>     fileSlabs         = std::nullopt;
        std::optional<std::vector<HyperSlab>>     mmrySlabs         = std::nullopt;
        std::optional<std::vector<H5S_seloper_t>> fileSlabSelectOps = std::nullopt;
        std::optional<std::vector<H5S_seloper_t>> mmrySlabSelectOps = std::nullopt;
    };

    /*!
     * \struct DataInfo
     * Struct with optional fields describing a C++ data type in memory
     */
    struct DataInfo {
        std::optional<size_t>               dataByte = std::nullopt;
        std::optional<hsize_t>              dataSize = std::nullopt;
        std::optional<int>                  dataRank = std::nullopt;
        std::optional<std::vector<hsize_t>> dataDims = std::nullopt;
        std::optional<hid::h5s>             h5_space = std::nullopt;
        std::optional<std::string>          cpp_type = std::nullopt;
        DataInfo()                                   = default;
        explicit DataInfo(const hid::h5s &space) {
            h5_space = space;
            setFromSpace();
        }
        void setFromSpace() {
            if(not h5_space) return;
            dataRank = H5Sget_simple_extent_ndims(h5_space.value());
            dataDims = std::vector<hsize_t>((size_t) dataRank.value(), 0);
            H5Sget_simple_extent_dims(h5_space.value(), dataDims->data(), nullptr);
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataByte) error_msg.append(" | dataByte");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5_space) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write from memory. The following fields are undefined\n\t" + error_msg);
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write from memory. The following fields are not valid\n\t" + error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), (hsize_t) 1, std::multiplies<>());
            if(size_check != dataSize.value()) throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check));
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not dataSize) error_msg.append(" | dataSize");
            if(not dataByte) error_msg.append(" | dataByte");
            if(not dataRank) error_msg.append(" | dataRank");
            if(not dataDims) error_msg.append(" | dataDims");
            if(not h5_space) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read into memory. The following fields are undefined\n\t" + error_msg);
            if(not h5_space->valid() ) error_msg.append(" | h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read into memory. The following fields are not valid\n\t" + error_msg);

            /* clang-format on */
            hsize_t size_check = std::accumulate(dataDims->begin(), dataDims->end(), (hsize_t) 1, std::multiplies<>());
            if(size_check != dataSize.value()) throw std::runtime_error(h5pp::format("Data size mismatch: dataSize [{}] | size check [{}]", dataSize.value(), size_check));
        }
        [[nodiscard]] std::string string() const {
            //            std::string msg;
            std::string msg;
            /* clang-format off */
            if(dataSize) msg.append(h5pp::format(" | size {}", dataSize.value()));
            if(dataByte) msg.append(h5pp::format(" | bytes {}", dataByte.value()));
            if(dataRank) msg.append(h5pp::format(" | rank {}", dataRank.value()));
            if(dataDims and not dataDims->empty())
                         msg.append(h5pp::format(" | dims {}", dataDims.value()));
            if(cpp_type) msg.append(h5pp::format(" | type [{}]", cpp_type.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct DsetInfo
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct DsetInfo {
        std::optional<hid::h5f>             h5_file              = std::nullopt;
        std::optional<hid::h5d>             h5_dset              = std::nullopt;
        std::optional<hid::h5t>             h5_type              = std::nullopt;
        std::optional<H5D_layout_t>         h5_layout            = std::nullopt;
        std::optional<hid::h5s>             h5_space             = std::nullopt;
        std::optional<hid::h5p>             h5_plist_dset_create = std::nullopt;
        std::optional<hid::h5p>             h5_plist_dset_access = std::nullopt;
        std::optional<std::string>          dsetPath             = std::nullopt;
        std::optional<bool>                 dsetExists           = std::nullopt;
        std::optional<hsize_t>              dsetSize             = std::nullopt;
        std::optional<size_t>               dsetByte             = std::nullopt;
        std::optional<int>                  dsetRank             = std::nullopt;
        std::optional<std::vector<hsize_t>> dsetDims             = std::nullopt;
        std::optional<std::vector<hsize_t>> dsetDimsMax          = std::nullopt;
        std::optional<std::vector<hsize_t>> chunkDims            = std::nullopt;
        std::optional<unsigned int>         compression          = std::nullopt;

        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_file             ) error_msg.append("\n\t h5_file");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_dset_create) error_msg.append("\n\t h5_plist_dset_create");
            if(not h5_plist_dset_access) error_msg.append("\n\t h5_plist_dset_access");
            if(not dsetPath            ) error_msg.append("\n\t dsetPath");
            if(not dsetExists          ) error_msg.append("\n\t dsetExists");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create dataset. The following fields are undefined\n\t" + error_msg);
            if(not h5_file->valid()             ) error_msg.append("\n\t h5_file");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid()            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_dset_create->valid()) error_msg.append("\n\t h5_plist_dset_create");
            if(not h5_plist_dset_access->valid()) error_msg.append("\n\t h5_plist_dset_access");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create dataset. The following fields are not valid\n\t" + error_msg);
            /* clang-format on */
        }
        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_dset             ) error_msg.append("\n\t h5_dset");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not dsetPath            ) error_msg.append("\n\t dsetPath");
            if(not dsetExists          ) error_msg.append("\n\t dsetExists");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write into dataset. The following fields are undefined\n\t" + error_msg);
            if(not h5_dset->valid() ) error_msg.append("\n\t h5_dset");
            if(not h5_type->valid() ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid() ) error_msg.append("\n\t h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot write into dataset. The following fields are not valid\n\t" + error_msg);
            /* clang-format on */
        }
        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_dset             ) error_msg.append("\n\t h5_dset");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not dsetPath            ) error_msg.append("\n\t dsetPath");
            if(not dsetExists          ) error_msg.append("\n\t dsetExists");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read from dataset. The following fields are undefined\n\t" + error_msg);
            if(not h5_file->valid() ) error_msg.append("\n\t h5_file");
            if(not h5_type->valid() ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid() ) error_msg.append("\n\t h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot read from dataset. The following fields are not valid\n\t" + error_msg);
            /* clang-format on */
        }
        [[nodiscard]] std::string string() const {
            //            std::string msg;
            std::string msg;
            /* clang-format off */
            if(dsetSize)    msg.append(h5pp::format(" | size {}", dsetSize.value()));
            if(dsetByte)    msg.append(h5pp::format(" | bytes {}", dsetByte.value()));
            if(dsetRank)    msg.append(h5pp::format(" | rank {}", dsetRank.value()));
            if(dsetDims and not dsetDims->empty())
                            msg.append(h5pp::format(" | dims {}", dsetDims.value()));
            if(dsetDimsMax and not dsetDimsMax->empty()){
                std::vector<long> maxDimsLong;
                for(auto &dim : dsetDimsMax.value()) {
                    if(dim == H5S_UNLIMITED)
                        maxDimsLong.emplace_back(-1);
                    else
                        maxDimsLong.emplace_back((long)dim);
                }
                msg.append(h5pp::format(" | max dims {}", maxDimsLong));
            }
            if(chunkDims)   msg.append(h5pp::format(" | chunk dims {}", chunkDims.value()));
            if(dsetPath)    msg.append(h5pp::format(" | path [{}]",dsetPath.value()));
            return msg;
            /* clang-format on */
        }
    };

    /*!
     * \struct AttrInfo
     * Struct with optional fields describing data on file, i.e. a dataset
     */
    struct AttrInfo {
        std::optional<hid::h5a>             h5_attr              = std::nullopt;
        std::optional<hid::h5o>             h5_link              = std::nullopt;
        std::optional<hid::h5t>             h5_type              = std::nullopt;
        std::optional<hid::h5s>             h5_space             = std::nullopt;
        std::optional<hid::h5p>             h5_plist_attr_create = std::nullopt;
        std::optional<hid::h5p>             h5_plist_attr_access = std::nullopt;
        std::optional<std::string>          attrName             = std::nullopt;
        std::optional<std::string>          linkPath             = std::nullopt;
        std::optional<bool>                 attrExists           = std::nullopt;
        std::optional<bool>                 linkExists           = std::nullopt;
        std::optional<hsize_t>              attrSize             = std::nullopt;
        std::optional<size_t>               attrByte             = std::nullopt;
        std::optional<int>                  attrRank             = std::nullopt;
        std::optional<std::vector<hsize_t>> attrDims             = std::nullopt;

        void assertCreateReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_link             ) error_msg.append("\n\t h5_link");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_attr_create) error_msg.append("\n\t h5_plist_attr_create");
            if(not h5_plist_attr_access) error_msg.append("\n\t h5_plist_attr_access");
            if(not attrName            ) error_msg.append("\n\t attrName");
            if(not linkPath            ) error_msg.append("\n\t linkPath");
            if(not attrExists          ) error_msg.append("\n\t attrExists");
            if(not linkExists          ) error_msg.append("\n\t linkExists");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create attribute. The following fields are undefined" + error_msg);
            if(not linkExists.value())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The link does not exist",attrName.value(),linkPath.value()));
            if(not h5_link->valid()             ) error_msg.append("\n\t h5_link");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not h5_space->valid()            ) error_msg.append("\n\t h5_space");
            if(not h5_plist_attr_create->valid()) error_msg.append("\n\t h5_plist_attr_create");
            if(not h5_plist_attr_access->valid()) error_msg.append("\n\t h5_plist_attr_access");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        void assertWriteReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_attr             ) error_msg.append("\n\t h5_attr");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create attribute. The following fields are undefined\n\t" + error_msg);
            if(not h5_attr->valid()             ) error_msg.append("\n\t h5_attr");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        void assertReadReady() const {
            std::string error_msg;
            /* clang-format off */
            if(not h5_attr             ) error_msg.append("\n\t h5_attr");
            if(not h5_type             ) error_msg.append("\n\t h5_type");
            if(not h5_space            ) error_msg.append("\n\t h5_space");
            if(not error_msg.empty())
                throw std::runtime_error("Cannot create attribute. The following fields are undefined\n\t" + error_msg);
            if(not h5_attr->valid()             ) error_msg.append("\n\t h5_attr");
            if(not h5_type->valid()             ) error_msg.append("\n\t h5_type");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create attribute [{}] for link [{}]. The following fields are not valid: {}",attrName.value(),linkPath.value(),error_msg));
            /* clang-format on */
        }

        [[nodiscard]] std::string string() const {
            std::string msg;
            /* clang-format off */
            if(attrSize) msg.append(h5pp::format(" | size {}", attrSize.value()));
            if(attrByte) msg.append(h5pp::format(" | bytes {}", attrByte.value()));
            if(attrRank) msg.append(h5pp::format(" | rank {}", attrRank.value()));
            if(attrDims and not attrDims->empty())
                         msg.append(h5pp::format(" | dims {}", attrDims.value()));
            if(attrName) msg.append(h5pp::format(" | name [{}]",attrName.value()));
            if(linkPath) msg.append(h5pp::format(" | link [{}]",linkPath.value()));
            return msg;
            /* clang-format on */
        }
    };


    /*!
    * \brief Information about tables
    */
    struct TableInfo {
        std::optional<size_t>                   numFields;
        std::optional<size_t>                   numRecords;
        std::optional<size_t>                   recordBytes;
        std::optional<std::vector<std::string>> fieldNames;
        std::optional<std::vector<size_t>>      fieldSizes;
        std::optional<std::vector<size_t>>      fieldOffsets;
        std::optional<std::vector<hid::h5t>>    fieldTypes;
        std::optional<bool>                     tableExists;
        std::optional<std::string>              tableTitle;
        std::optional<std::string>              tableName;
        std::optional<std::string>              tableGroupName;
        std::optional<hid::h5f>                 tableFile;
        std::optional<hid::h5g>                 tableGroup;
        std::optional<hid_t>                    tableLocId;
        std::optional<hid::h5d>                 tableDset;
        std::optional<hid::h5t>                 tableType;
        std::optional<size_t>                   compressionLevel;
        std::optional<hsize_t>                  chunkSize;
        void assertCreateReady() const {
            std::string error_msg;
            if(not numFields        ) error_msg.append("\n\t numFields");
            if(not numRecords       ) error_msg.append("\n\t numRecords");
            if(not recordBytes      ) error_msg.append("\n\t recordBytes");
            if(not fieldNames       ) error_msg.append("\n\t fieldNames");
            if(not fieldSizes       ) error_msg.append("\n\t fieldSizes");
            if(not fieldOffsets     ) error_msg.append("\n\t fieldOffsets");
            if(not fieldTypes       ) error_msg.append("\n\t fieldTypes");
            if(not tableName        ) error_msg.append("\n\t tableName");
            if(not tableGroupName   ) error_msg.append("\n\t tableGroupName");
            if(not tableTitle       ) error_msg.append("\n\t tableTitle");
            if(not compressionLevel ) error_msg.append("\n\t compressionLevel");
            if(not chunkSize        ) error_msg.append("\n\t chunkSize");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot create new table: The following fields are not set: {}", error_msg));
        }
        void assertReadReady() const {
            std::string error_msg;
            if(not recordBytes  ) error_msg.append("\n\t recordBytes");
            if(not fieldSizes   ) error_msg.append("\n\t fieldSizes");
            if(not fieldOffsets ) error_msg.append("\n\t fieldOffsets");
            if(not tableName    ) error_msg.append("\n\t tableName");
            if(not tableLocId   ) error_msg.append("\n\t tableLocId");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot read from table: The following fields are not set: {}", error_msg));

        }
        void assertWriteReady() const {
            std::string error_msg;
            if(not recordBytes  ) error_msg.append("\n\t recordBytes");
            if(not fieldSizes   ) error_msg.append("\n\t fieldSizes");
            if(not fieldOffsets ) error_msg.append("\n\t fieldOffsets");
            if(not tableName    ) error_msg.append("\n\t tableName");
            if(not tableLocId   ) error_msg.append("\n\t tableLocId");
            if(not error_msg.empty())
                throw std::runtime_error(h5pp::format("Cannot write to table: The following fields are not set: {}", error_msg));
        }
    };



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


}
